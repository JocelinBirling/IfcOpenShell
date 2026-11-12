// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "View.h"

#include "ReportDialog.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../components/Panel.h"
#include "../../ports/ifctester/Ids.h"
#include "../../services/ActionPipeline.h"
#include "../../services/AppearanceService.h"
#include "../../services/IfcDiff/DiffMarks.h"
#include "../../services/PopupService.h"
#include "../../services/SelectionService.h"
#include "../../services/UndoStack.h"
#include "../../../ifcviewer/Federation.h"
#include "../../../ifcviewer/SceneLoader.h"
#include "../../../ifcparse/file.h"

#include <QColor>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTimer>

#include <algorithm>

namespace bonsaiviewer::modules::diff {

namespace df = bonsaiviewer::services::ifcdiff;
using Layer = bonsaiviewer::services::AppearanceService::Layer;

namespace {

// Spec 34 colours.
const QColor kDeleted("#E6194B");
const QColor kAdded("#3CB44B");
const QColor kModifiedBefore("#3B6FE0");
const QColor kModifiedAfter("#F08A24");
constexpr int kTransparentPercent = 50;

QString text(const std::string& value) {
    return QString::fromStdString(value);
}

QString joinGuids(const std::vector<std::string>& guids) {
    QStringList list;
    for (const std::string& guid : guids) list << text(guid);
    return list.join(", ");
}

QString point(const std::array<double, 3>& p) {
    return QString("(%1, %2, %3)").arg(p[0], 0, 'g', 10).arg(p[1], 0, 'g', 10).arg(p[2], 0, 'g', 10);
}

QString kindLabel(const df::PropertyChange& change) {
    QString label = text(df::changeKindName(change.kind));
    if (change.direction > 0) label += " (increased)";
    else if (change.direction < 0) label += " (decreased)";
    return label;
}

bool matchesKind(const df::PropertyChange& change, SelectKind kind) {
    using K = df::PropertyChangeKind;
    switch (kind) {
    case SelectKind::PropertyAdded: return change.kind == K::PropertyAdded;
    case SelectKind::PropertyRemoved: return change.kind == K::PropertyRemoved;
    case SelectKind::ValueAdded: return change.kind == K::ValueAdded;
    case SelectKind::ValueRemoved: return change.kind == K::ValueRemoved;
    case SelectKind::ValueChanged: return change.kind == K::ValueChanged;
    case SelectKind::ValueIncreased: return change.kind == K::ValueChanged && change.direction > 0;
    case SelectKind::ValueDecreased: return change.kind == K::ValueChanged && change.direction < 0;
    }
    return false;
}

bool matchesGeometry(const df::ElementChange& change, GeometryKind kind) {
    if (!change.geometry_changed) return false;
    const df::GeometryDetail& g = change.geometry;
    switch (kind) {
    case GeometryKind::Any: return true;
    case GeometryKind::Moved: return g.moved;
    case GeometryKind::Rotated: return g.rotation_changed;
    case GeometryKind::Shape: return g.representation_changed;
    case GeometryKind::Openings:
        return !g.openings_added.empty() || !g.openings_removed.empty() || !g.projections_added.empty() ||
               !g.projections_removed.empty();
    }
    return false;
}

QString geometryKindLabel(GeometryKind kind) {
    switch (kind) {
    case GeometryKind::Any: return "a geometry change";
    case GeometryKind::Moved: return "a move";
    case GeometryKind::Rotated: return "a rotation";
    case GeometryKind::Shape: return "a shape change";
    case GeometryKind::Openings: return "an openings change";
    }
    return {};
}

QString selectKindLabel(SelectKind kind) {
    switch (kind) {
    case SelectKind::PropertyAdded: return "a property added";
    case SelectKind::PropertyRemoved: return "a property removed";
    case SelectKind::ValueAdded: return "a value added";
    case SelectKind::ValueRemoved: return "a value removed";
    case SelectKind::ValueChanged: return "a value changed";
    case SelectKind::ValueIncreased: return "a value increased";
    case SelectKind::ValueDecreased: return "a value decreased";
    }
    return {};
}

} // namespace

DiffPanelView::DiffPanelView(components::Panel* panel,
                             SessionState& session,
                             services::SelectionService& selection,
                             services::ActionPipeline& pipeline,
                             services::AppearanceService& appearance,
                             services::UndoStack& undo,
                             services::PropertyCatalog& catalog,
                             df::DiffMarks& marks,
                             QObject* parent)
    : QObject(parent),
      session_(session),
      selection_(selection),
      pipeline_(pipeline),
      appearance_(appearance),
      undo_(undo),
      marks_(marks)
{
    panel_ = new DiffPanel(catalog, panel);
    panel->addBodyWidget(panel_);
    refreshModels();

    connect(panel_, &DiffPanel::runRequested, this, [this]() {
        if (running_) return;
        panel_->setRunning(true);
        QTimer::singleShot(0, this, &DiffPanelView::run); // let "Running…" show first
    });
    connect(panel_, &DiffPanel::exportRequested, this, &DiffPanelView::exportJson);
    connect(panel_, &DiffPanel::clearRequested, this, [this]() {
        withUndo([this]() { clearColours(); });
        publishMarks(false);
        session_.setStatusMessage("IfcDiff", "Diff colours cleared (CTRL+Z brings them back)");
    });
    connect(panel_, &DiffPanel::swapTransparencyRequested, this, &DiffPanelView::swapTransparency);
    connect(panel_, &DiffPanel::selectRequested, this, &DiffPanelView::select);
    connect(panel_, &DiffPanel::geometrySelectRequested, this, &DiffPanelView::selectGeometry);
    connect(panel_, &DiffPanel::caseSelectRequested, this, &DiffPanelView::selectCase);
    connect(panel_, &DiffPanel::reportRequested, this, &DiffPanelView::showReport);
    connect(panel_, &DiffPanel::compareRequested, this, &DiffPanelView::compareSelection);

    // The panel state (models, relationships, IDS) stays for the session;
    // the result and its colours go when a model is loaded or removed.
    connect(&session_, &SessionState::modelsChanged, this, [this]() {
        if (result_) {
            dropResult();
            panel_->setSummary("The models changed: run the comparison again.");
        }
        refreshModels();
    });
    connect(&session_, &SessionState::modelDataSourceReady, this, [this](std::uint32_t) { refreshModels(); });
    connect(&session_, &SessionState::projectReset, this, [this]() {
        result_.reset();
        marks_.clear();
        if (report_) report_->close();
        panel_->setResultAvailable(false);
        panel_->setSummary(QString());
        refreshModels();
    });
}

QString DiffPanelView::modelName(std::uint32_t session_model_id) const {
    const QString model_id = session_.modelIdForSessionModelId(session_model_id);
    if (auto* federation = session_.federation()) {
        if (const auto* model = federation->findById(model_id); model && !model->display_name.isEmpty()) {
            return model->display_name;
        }
    }
    auto* loader = session_.loader();
    return loader ? loader->displayName(session_model_id) : QString();
}

void DiffPanelView::refreshModels() {
    QList<QPair<std::uint32_t, QString>> models;
    for (std::uint32_t session_model_id : session_.sessionModelIds()) {
        if (session_model_id == 0) continue;
        models.append({session_model_id, modelName(session_model_id)});
    }
    panel_->setModels(models);
}

QHash<int, std::uint32_t> DiffPanelView::objectIdsOf(std::uint32_t session_model_id) const {
    QHash<int, std::uint32_t> ids;
    if (auto* registry = session_.elementRegistry()) {
        for (const auto& info : registry->basicElementInfoForModel(session_model_id)) ids.insert(info.ifc_id, info.object_id);
    }
    return ids;
}

std::vector<std::uint32_t> DiffPanelView::objectsOf(const df::ElementChange& change) const {
    std::vector<std::uint32_t> ids;
    if (const auto found = before_objects_.find(change.old_id); found != before_objects_.end()) ids.push_back(found.value());
    if (const auto found = after_objects_.find(change.new_id); found != after_objects_.end()) ids.push_back(found.value());
    return ids;
}

void DiffPanelView::withUndo(const std::function<void()>& apply) {
    const bool was_suspended = undo_.isSuspended();
    undo_.record();
    undo_.setSuspended(true);
    apply();
    undo_.setSuspended(was_suspended);
}

void DiffPanelView::run() {
    running_ = true;
    const auto finish = [this]() {
        running_ = false;
        panel_->setRunning(false);
    };
    const std::uint32_t before = panel_->beforeModel();
    const std::uint32_t after = panel_->afterModel();
    if (before == 0 || after == 0) {
        services::popup::information(panel_, "Choose a Before model and an After model to compare.");
        return finish();
    }
    if (before == after) {
        services::popup::information(panel_, "The Before and After models are the same model: choose two models.");
        return finish();
    }
    auto* loader = session_.loader();
    ifcopenshell::file* old_file = loader ? loader->ifcFile(before) : nullptr;
    ifcopenshell::file* new_file = loader ? loader->ifcFile(after) : nullptr;
    if (old_file == nullptr || new_file == nullptr) {
        services::popup::information(panel_, "The IFC data of a model is not open yet: wait for the model to load, then run again.");
        return finish();
    }
    const std::set<std::string> relationships = panel_->relationships();
    if (relationships.empty()) {
        services::popup::information(panel_, "Check at least one relationship to compare.");
        return finish();
    }

    df::Options options;
    options.relationships = relationships;
    options.is_shallow = panel_->isShallow();
    if (!panel_->idsPath().isEmpty()) {
        QFile file(panel_->idsPath());
        std::string error;
        std::optional<bonsaiviewer::ports::ifctester::Ids> ids;
        if (file.open(QIODevice::ReadOnly)) ids = bonsaiviewer::ports::ifctester::idsFromXml(file.readAll(), &error);
        else error = file.errorString().toStdString();
        if (!ids) {
            services::popup::information(panel_, QString("The IDS shortlist cannot be read: %1").arg(text(error)));
            return finish();
        }
        options.ids = std::move(ids);
    }
    session_.beginProgress("IfcDiff");
    session_.setStatusMessage("IfcDiff", QString("Comparing %1 and %2…").arg(modelName(before), modelName(after)));
    options.progress = [this](int percent) {
        session_.setProgress(percent);
        // The run is on the GUI thread (the IFC files are shared with the
        // panels): keep the progress bar painted, but no user input.
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    };

    QElapsedTimer timer;
    timer.start();
    df::Result result;
    try {
        result = df::diff(*old_file, *new_file, options);
    } catch (const std::exception& e) {
        session_.endProgress();
        services::popup::information(panel_, QString("The comparison failed: %1").arg(e.what()));
        return finish();
    }
    session_.endProgress();

    // A new run replaces the previous one: one undo state brings back the
    // previous colours, then the old colours go and the new ones come.
    const bool was_suspended = undo_.isSuspended();
    undo_.record();
    undo_.setSuspended(true);
    clearColours();
    marks_.clear();
    result_ = std::move(result);
    before_model_ = before;
    after_model_ = after;
    before_objects_ = objectIdsOf(before);
    after_objects_ = objectIdsOf(after);
    deleted_ids_.clear();
    added_ids_.clear();
    modified_before_ids_.clear();
    modified_after_ids_.clear();
    for (const df::ElementRef& element : result_->deleted) {
        if (const auto found = before_objects_.find(element.id); found != before_objects_.end()) deleted_ids_.push_back(found.value());
    }
    for (const df::ElementRef& element : result_->added) {
        if (const auto found = after_objects_.find(element.id); found != after_objects_.end()) added_ids_.push_back(found.value());
    }
    for (const auto& [guid, change] : result_->changed) {
        if (const auto found = before_objects_.find(change.old_id); found != before_objects_.end()) modified_before_ids_.push_back(found.value());
        if (const auto found = after_objects_.find(change.new_id); found != after_objects_.end()) modified_after_ids_.push_back(found.value());
    }
    applyColours();
    undo_.setSuspended(was_suspended);
    publishMarks(true);
    panel_->setResultAvailable(true);
    const QString summary = QString("%1 added · %2 deleted · %3 changed (of %4 / %5 elements, %6 s)")
                                .arg(result_->added.size())
                                .arg(result_->deleted.size())
                                .arg(result_->changed.size())
                                .arg(result_->old_count)
                                .arg(result_->new_count)
                                .arg(timer.elapsed() / 1000.0, 0, 'f', 1);
    panel_->setSummary(summary);
    session_.setStatusMessage("IfcDiff", summary);
    if (report_ && report_->isVisible()) showReport();
    finish();
}

void DiffPanelView::dropResult() {
    // Not a display action of the user (a model was loaded or removed): no
    // undo state.
    if (result_) {
        const bool was_suspended = undo_.isSuspended();
        undo_.setSuspended(true);
        clearColours();
        undo_.setSuspended(was_suspended);
    }
    marks_.clear();
    result_.reset();
    deleted_ids_.clear();
    added_ids_.clear();
    modified_before_ids_.clear();
    modified_after_ids_.clear();
    panel_->setResultAvailable(false);
    if (report_) report_->close();
}

void DiffPanelView::applyColours() {
    appearance_.clearLayer(Layer::DiffResult);
    appearance_.setLayerColour(deleted_ids_, kDeleted, Layer::DiffResult);
    appearance_.setLayerColour(added_ids_, kAdded, Layer::DiffResult);
    appearance_.setLayerColour(modified_before_ids_, kModifiedBefore, Layer::DiffResult);
    appearance_.setLayerColour(modified_after_ids_, kModifiedAfter, Layer::DiffResult);
    appearance_.setOpacity(before_transparent_ ? modified_after_ids_ : modified_before_ids_, 100);
    appearance_.setOpacity(before_transparent_ ? modified_before_ids_ : modified_after_ids_, kTransparentPercent);
    panel_->setBeforeTransparent(before_transparent_);
}

void DiffPanelView::clearColours() {
    appearance_.clearLayer(Layer::DiffResult);
    appearance_.setOpacity(modified_before_ids_, 100);
    appearance_.setOpacity(modified_after_ids_, 100);
}

void DiffPanelView::swapTransparency() {
    if (!result_) return;
    before_transparent_ = !before_transparent_;
    withUndo([this]() {
        appearance_.setOpacity(before_transparent_ ? modified_after_ids_ : modified_before_ids_, 100);
        appearance_.setOpacity(before_transparent_ ? modified_before_ids_ : modified_after_ids_, kTransparentPercent);
    });
    panel_->setBeforeTransparent(before_transparent_);
    session_.setStatusMessage("IfcDiff", before_transparent_ ? "Modified Before elements at 50 % opacity"
                                                             : "Modified After elements at 50 % opacity");
}

void DiffPanelView::publishMarks(bool on) {
    if (!on || !result_) {
        marks_.clear();
        return;
    }
    std::map<df::DiffMarks::Key, df::DiffMarks::Mark> changes;
    for (const auto& [guid, change] : result_->changed) {
        if (change.properties.empty() && change.attributes.empty()) continue;
        changes[{before_model_, change.old_id}] = {&change, true};
        changes[{after_model_, change.new_id}] = {&change, false};
    }
    marks_.setChanges(std::move(changes));
}

void DiffPanelView::select(SelectKind kind, bool isolate) {
    if (!result_) return;
    const QString pset = panel_->propertySet();
    const QString property = panel_->propertyName();
    std::vector<std::uint32_t> ids;
    int elements = 0;
    for (const auto& [guid, change] : result_->changed) {
        const bool matches = std::any_of(change.properties.begin(), change.properties.end(), [&](const df::PropertyChange& p) {
            if (!pset.isEmpty() && text(p.pset) != pset) return false;
            if (!property.isEmpty() && text(p.property) != property) return false;
            return matchesKind(p, kind);
        });
        if (!matches) continue;
        ++elements;
        const auto objects = objectsOf(change);
        ids.insert(ids.end(), objects.begin(), objects.end());
    }
    const QString scope = property.isEmpty() ? QString() : QString(" on %1").arg(pset.isEmpty() ? property : pset + "." + property);
    if (ids.empty()) {
        session_.setStatusMessage("IfcDiff", QString("No element with %1%2").arg(selectKindLabel(kind), scope));
        return;
    }
    if (isolate) pipeline_.run(ids, services::ActionPipeline::Action::Isolate);
    else selection_.setSelection(ids, services::SelectionSource::Service, services::SelectionMode::Replace);
    session_.setStatusMessage("IfcDiff", QString("%1 element(s) with %2%3 %4")
                                             .arg(elements)
                                             .arg(selectKindLabel(kind), scope, isolate ? "isolated" : "selected"));
}

void DiffPanelView::selectGeometry(GeometryKind kind, bool isolate) {
    if (!result_) return;
    std::vector<std::uint32_t> ids;
    int elements = 0;
    for (const auto& [guid, change] : result_->changed) {
        if (!matchesGeometry(change, kind)) continue;
        ++elements;
        const auto objects = objectsOf(change);
        ids.insert(ids.end(), objects.begin(), objects.end());
    }
    if (ids.empty()) {
        const bool compared = panel_->relationships().count("geometry") > 0;
        session_.setStatusMessage("IfcDiff", compared ? QString("No element with %1").arg(geometryKindLabel(kind))
                                                      : QString("The geometry was not compared: check \"geometry\" and run again"));
        return;
    }
    if (isolate) pipeline_.run(ids, services::ActionPipeline::Action::Isolate);
    else selection_.setSelection(ids, services::SelectionSource::Service, services::SelectionMode::Replace);
    session_.setStatusMessage("IfcDiff", QString("%1 element(s) with %2 %3")
                                             .arg(elements)
                                             .arg(geometryKindLabel(kind), isolate ? "isolated" : "selected"));
}

void DiffPanelView::selectCase(DiffCase kind, bool isolate) {
    if (!result_) return;
    // The object ids coloured by the run (elements with geometry only).
    const std::vector<std::uint32_t>* ids = nullptr;
    QString label;
    switch (kind) {
    case DiffCase::Deleted: ids = &deleted_ids_; label = "deleted (Before)"; break;
    case DiffCase::Added: ids = &added_ids_; label = "added (After)"; break;
    case DiffCase::ModifiedBefore: ids = &modified_before_ids_; label = "modified (Before)"; break;
    case DiffCase::ModifiedAfter: ids = &modified_after_ids_; label = "modified (After)"; break;
    }
    if (ids->empty()) {
        session_.setStatusMessage("IfcDiff", QString("No %1 element in the 3D view").arg(label));
        return;
    }
    if (isolate) pipeline_.run(*ids, services::ActionPipeline::Action::Isolate);
    else selection_.setSelection(*ids, services::SelectionSource::Service, services::SelectionMode::Replace);
    session_.setStatusMessage("IfcDiff", QString("%1 %2 element(s) %3").arg(ids->size()).arg(label, isolate ? "isolated" : "selected"));
}

void DiffPanelView::showReport() {
    if (!result_) return;
    const QString before = "Before";
    const QString after = "After";
    std::vector<ReportRow> rows;
    const auto element_row = [](const QString& model, const std::string& guid, const std::string& ifc_class,
                                const std::string& name, const QString& changes) {
        ReportRow row;
        row.model = model;
        row.global_id = text(guid);
        row.ifc_class = text(ifc_class);
        row.name = text(name);
        row.changes = changes;
        return row;
    };
    for (const df::ElementRef& element : result_->deleted) {
        ReportRow row = element_row(before, element.global_id, element.ifc_class, element.name, "deleted");
        if (const auto found = before_objects_.find(element.id); found != before_objects_.end()) row.object_ids.push_back(found.value());
        rows.push_back(row);
    }
    for (const df::ElementRef& element : result_->added) {
        ReportRow row = element_row(after, element.global_id, element.ifc_class, element.name, "added");
        if (const auto found = after_objects_.find(element.id); found != after_objects_.end()) row.object_ids.push_back(found.value());
        rows.push_back(row);
    }
    for (const auto& [guid, change] : result_->changed) {
        QStringList kinds;
        for (const std::string& kind : change.kinds()) kinds << text(kind);
        ReportRow base = element_row(before + " + " + after, guid, change.ifc_class, change.name, kinds.join(", "));
        base.object_ids = objectsOf(change);
        const auto add = [&rows, &base](const QString& group, const QString& property, const QString& old_value,
                                        const QString& new_value, const QString& kind) {
            ReportRow row = base;
            row.group = group;
            row.property = property;
            row.old_value = old_value;
            row.new_value = new_value;
            row.kind = kind;
            rows.push_back(std::move(row));
        };
        const std::size_t first = rows.size();
        for (const df::AttributeChange& attribute : change.attributes) {
            add("Attributes", text(attribute.name), text(attribute.old_value), text(attribute.new_value), "attribute changed");
        }
        for (const df::PropertyChange& property : change.properties) {
            add(text(property.pset), text(property.property), text(property.old_value), text(property.new_value), kindLabel(property));
        }
        const df::GeometryDetail& geometry = change.geometry;
        if (change.geometry_changed) {
            if (geometry.placement_changed) {
                add("Geometry", "Placement", point(geometry.translation_old), point(geometry.translation_new),
                    geometry.moved && geometry.rotation_changed ? "moved and rotated"
                                                                : (geometry.moved ? "moved" : "rotated"));
            }
            if (!geometry.openings_added.empty()) add("Geometry", "Openings", QString(), joinGuids(geometry.openings_added), "openings added");
            if (!geometry.openings_removed.empty()) add("Geometry", "Openings", joinGuids(geometry.openings_removed), QString(), "openings removed");
            if (!geometry.projections_added.empty()) add("Geometry", "Projections", QString(), joinGuids(geometry.projections_added), "projections added");
            if (!geometry.projections_removed.empty()) add("Geometry", "Projections", joinGuids(geometry.projections_removed), QString(), "projections removed");
            if (geometry.items_count_old != geometry.items_count_new) {
                add("Geometry", "Body items", QString::number(geometry.items_count_old), QString::number(geometry.items_count_new),
                    "representation changed");
            }
            for (const df::ItemChange& item : geometry.items_changed) {
                const QString prefix = QString("Item %1 %2").arg(item.index).arg(text(item.type_new));
                if (item.attributes.empty()) add("Geometry", prefix, text(item.type_old), text(item.type_new), "representation changed");
                for (const df::ItemAttributeChange& attribute : item.attributes) {
                    add("Geometry", prefix + ": " + text(attribute.path), text(attribute.old_value), text(attribute.new_value),
                        "representation changed");
                }
            }
            if (rows.size() == first + change.attributes.size() + change.properties.size()) {
                add("Geometry", QString(), QString(), QString(), "geometry changed");
            }
        }
        const std::pair<bool, const char*> relations[] = {{change.type_changed, "Type"},
                                                          {change.container_changed, "Container"},
                                                          {change.aggregate_changed, "Aggregate"},
                                                          {change.classification_changed, "Classification"}};
        for (const auto& [changed, label] : relations) {
            if (changed) add("Relationships", label, QString(), QString(), "changed");
        }
        if (rows.size() == first) rows.push_back(base);
    }

    if (!report_) {
        report_ = new ReportDialog(panel_->window());
        connect(report_, &ReportDialog::selectRequested, this, [this](const std::vector<std::uint32_t>& ids) {
            selection_.setSelection(ids, services::SelectionSource::Table, services::SelectionMode::Replace);
        });
        connect(report_, &ReportDialog::isolateRequested, this, [this](const std::vector<std::uint32_t>& ids) {
            if (ids.empty()) {
                session_.setStatusMessage("IfcDiff", "The element has no geometry in the viewer");
                return;
            }
            pipeline_.run(ids, services::ActionPipeline::Action::Isolate);
        });
        connect(report_, &ReportDialog::compareRequested, this, &DiffPanelView::compare);
    }
    report_->setRows(std::move(rows));
    report_->show();
    report_->raise();
    report_->activateWindow();
}

void DiffPanelView::compare(const QString& global_id) {
    if (!result_) return;
    auto* loader = session_.loader();
    ifcopenshell::file* old_file = loader ? loader->ifcFile(before_model_) : nullptr;
    ifcopenshell::file* new_file = loader ? loader->ifcFile(after_model_) : nullptr;
    const auto find = [&global_id](ifcopenshell::file* file) -> express::base {
        if (file == nullptr) return {};
        try {
            return file->instance_by_guid(global_id.toStdString());
        } catch (...) {
            return {};
        }
    };
    const express::base before = find(old_file);
    const express::base after = find(new_file);
    if (!before && !after) {
        session_.setStatusMessage("IfcDiff", QString("%1 is in neither model").arg(global_id));
        return;
    }
    auto* dialog = new CompareDialog(modelName(before_model_), before, modelName(after_model_), after, panel_->window());
    dialog->show();
}

void DiffPanelView::compareSelection() {
    if (!result_) return;
    auto* registry = session_.elementRegistry();
    const std::uint32_t object_id = selection_.activeObjectId();
    const auto info = registry && object_id != 0 ? registry->findBasicElementInfo(object_id) : std::nullopt;
    if (!info || info->guid.isEmpty()) {
        services::popup::information(panel_, "Select an element of the Before or After model to compare.");
        return;
    }
    if (info->session_model_id != before_model_ && info->session_model_id != after_model_) {
        services::popup::information(panel_, "The selected element is not in the compared models.");
        return;
    }
    compare(info->guid);
}

void DiffPanelView::exportJson() {
    if (!result_) {
        services::popup::information(panel_, "Run the comparison first.");
        return;
    }
    const QString path = QFileDialog::getSaveFileName(panel_, "Export the IfcDiff result", "diff.json", "JSON files (*.json)");
    if (path.isEmpty()) return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        services::popup::information(panel_, QString("Cannot write %1: %2").arg(path, file.errorString()));
        return;
    }
    file.write(QJsonDocument(result_->toJson()).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        services::popup::information(panel_, QString("Cannot write %1: %2").arg(path, file.errorString()));
        return;
    }
    session_.setStatusMessage("IfcDiff", QString("Exported to %1").arg(path));
}

} // namespace bonsaiviewer::modules::diff
