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

#include "ClashView.h"

#include "RuleEditDialog.h"
#include "RulePanels.h"
#include "RuleTree.h"

#include "../../SessionState.h"
#include "../../components/Buttons.h"
#include "../../services/ActionPipeline.h"
#include "../../services/ActiveView.h"
#include "../../services/AppearanceService.h"
#include "../../services/FilterRunner.h"
#include "../../services/IdsBridge.h"
#include "../../services/PopupService.h"
#include "../../services/SelectionService.h"
#include "../../services/UndoStack.h"
#include "../../services/clash/RuleCatalogue.h"
#include "../../services/clash/RuleJson.h"
#include "../viewport/Commands.h"
#include "../../../ifcviewer/OverlayRenderer.h"
#include "../../../ifcviewer/SceneLoader.h"
#include "../../../ifcviewer/ViewportWindow.h"

#include "api/Engine.h"

#include <QActionGroup>
#include <QColor>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMenu>
#include <QMessageBox>
#include <QSaveFile>
#include <QSettings>
#include <QToolButton>

#include <Eigen/Dense>

#include <algorithm>
#include <filesystem>
#include <set>
#include <tuple>

namespace bonsaiviewer::modules::rule {

namespace {

constexpr auto kLastDirectoryKey = "clash/lastDirectory";
using Layer = bonsaiviewer::services::AppearanceService::Layer;

const QColor kSourceColour("#E6194B");
const QColor kTargetColour("#F08A24");
const QColor kPassColour("#3CB44B");

// The engine works in the IFC world coordinates of the model: the overlay
// goes through the model's federation matrix (TI09).
struct Transform {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    void push(std::vector<float>& out, const ifcclash::Point3& p) const {
        const Eigen::Vector4d v = m * Eigen::Vector4d(p[0], p[1], p[2], 1.0);
        out.push_back(static_cast<float>(v.x()));
        out.push_back(static_cast<float>(v.y()));
        out.push_back(static_cast<float>(v.z()));
    }
    void segment(std::vector<float>& out, const ifcclash::Point3& a, const ifcclash::Point3& b) const {
        push(out, a);
        push(out, b);
    }
};

void obbEdges(const Transform& t, const ifcclash::ObbData& obb, std::vector<float>& out) {
    std::array<ifcclash::Point3, 8> corners;
    for (int i = 0; i < 8; ++i) {
        const double signs[3] = {(i & 1) ? 1.0 : -1.0, (i & 2) ? 1.0 : -1.0, (i & 4) ? 1.0 : -1.0};
        for (int c = 0; c < 3; ++c) {
            double v = obb.center[static_cast<size_t>(c)];
            for (int a = 0; a < 3; ++a) v += signs[a] * obb.half_sizes[static_cast<size_t>(a)] * obb.axes[static_cast<size_t>(a)][static_cast<size_t>(c)];
            corners[static_cast<size_t>(i)][static_cast<size_t>(c)] = v;
        }
    }
    static constexpr int kEdges[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {3, 7}, {2, 6}};
    for (const auto& e : kEdges) t.segment(out, corners[static_cast<size_t>(e[0])], corners[static_cast<size_t>(e[1])]);
}

OverlayRenderer::LineGroup lineGroup(const QColor& colour, float width, float dash = 0.0f) {
    OverlayRenderer::LineGroup group;
    group.color[0] = static_cast<float>(colour.redF());
    group.color[1] = static_cast<float>(colour.greenF());
    group.color[2] = static_cast<float>(colour.blueF());
    group.color[3] = 0.95f;
    group.line_width = width;
    group.dash_period_px = dash;
    return group;
}

bool isIfcPath(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return (suffix == QLatin1String("ifc") || suffix == QLatin1String("ifcxml")) && QFileInfo::exists(path);
}

} // namespace

ClashView::ClashView(Services services, QWidget* host, QObject* parent) : QObject(parent), s_(services), host_(host) {
    rule_panel_ = new RulePanel(document_, host_);
    library_ = new RuleLibraryPanel(host_);
    config_ = new RuleConfigPanel(document_, host_);
    results_ = new ResultPanel(host_);
    info_ = new ResultInfoPanel(host_);
    geometry_ribbon_ = buildGeometryRuleRibbon();
    filter_ribbon_ = buildClashFilterRibbon();
    document_.newFile();

    RuleTreeWidget* tree = rule_panel_->tree();
    connect(tree, &RuleTreeWidget::refSelected, config_, &RuleConfigPanel::showRef);
    connect(library_, &RuleLibraryPanel::ruleTypeSelected, config_, &RuleConfigPanel::showLibraryRule);
    connect(tree, &RuleTreeWidget::runRequested, this, &ClashView::run);
    connect(tree, &RuleTreeWidget::editRuleRequested, this, &ClashView::editRule);
    connect(tree, &RuleTreeWidget::editFolderRequested, this, &ClashView::editFolder);
    connect(config_, &RuleConfigPanel::editRuleRequested, this, &ClashView::editRule);
    connect(config_, &RuleConfigPanel::editFolderRequested, this, &ClashView::editFolder);
    connect(tree, &RuleTreeWidget::selectionRequested, this, &ClashView::showSelection);
    connect(rule_panel_, &RulePanel::saveRequested, this, [this]() { saveRules(false); });
    connect(rule_panel_, &RulePanel::loadRequested, this, &ClashView::loadRules);

    connect(results_, &ResultPanel::currentResultChanged, this, [this](int row) {
        info_->showRow(row >= 0 ? &results_->rows()[static_cast<size_t>(row)] : nullptr);
        drawOverlay(row);
    });
    connect(results_, &ResultPanel::rowsSelected, this, [this](const std::vector<int>& rows) {
        std::vector<std::uint32_t> ids;
        for (int row : rows) {
            const auto objects = results_->rows()[static_cast<size_t>(row)].objects();
            ids.insert(ids.end(), objects.begin(), objects.end());
        }
        s_.selection.setSelection(ids, services::SelectionSource::Table, services::SelectionMode::Replace);
    });
    connect(results_, &ResultPanel::isolateRequested, this, &ClashView::isolateRows);
    connect(results_, &ResultPanel::showRequested, this, [this](const std::vector<int>& rows) {
        std::vector<std::uint32_t> ids;
        for (int row : rows) {
            const auto objects = results_->rows()[static_cast<size_t>(row)].objects();
            ids.insert(ids.end(), objects.begin(), objects.end());
        }
        s_.view.add(ids);
        s_.selection.setSelection(ids, services::SelectionSource::Table, services::SelectionMode::Replace);
    });

    // The results point at the objects of the loaded models: dropped with them.
    const auto drop = [this]() {
        engine_key_.clear();
        if (busy_) return;
        engine_.reset();
        clearOverlay();
        if (!results_->rows().empty()) {
            clearColours();
            results_->setResults({});
            results_->setMessages({});
            info_->showRow(nullptr);
            rule_panel_->tree()->setResultTexts({});
        }
    };
    connect(&s_.session, &SessionState::modelsChanged, this, drop);
    connect(&s_.session, &SessionState::projectReset, this, drop);
}

ClashView::~ClashView() {
    if (cancel_) cancel_->cancel();
    if (worker_.joinable()) worker_.join();
}

// --- runs ------------------------------------------------------------------------------------

bool ClashView::prepareModels(QString* error) {
    auto* loader = s_.session.loader();
    run_models_.clear();
    run_paths_.clear();
    QStringList key;
    QStringList skipped;
    for (std::uint32_t id : s_.session.sessionModelIds()) {
        const QString path = loader ? loader->filePath(id) : QString();
        if (!isIfcPath(path)) {
            skipped << (loader ? loader->displayName(id) : QString::number(id));
            continue;
        }
        run_models_.push_back(id);
        run_paths_ << path;
        key << path + "@" + QString::number(QFileInfo(path).lastModified().toMSecsSinceEpoch());
    }
    if (run_models_.empty()) {
        *error = skipped.isEmpty() ? QStringLiteral("Load an IFC model before running the rules.")
                                   : QStringLiteral("The rules run on IFC files: the loaded models have no IFC file on disk.");
        return false;
    }
    if (!skipped.isEmpty()) run_messages_ << "Left out (no IFC file on disk): " + skipped.join(", ");
    if (key != engine_key_) {
        engine_.reset(); // the worker opens a new session
        engine_key_ = key;
    }
    elements_.clear();
    if (auto* registry = s_.session.elementRegistry()) {
        for (std::uint32_t id : run_models_) {
            QHash<int, BasicElementInfo> by_id;
            for (const BasicElementInfo& info : registry->basicElementInfoForModel(id)) by_id.insert(info.ifc_id, info);
            elements_.insert(id, by_id);
        }
    }
    return true;
}

bool ClashView::startWork(const QString& label, std::function<void(ifcclash::Session&, size_t)> work) {
    if (busy_) {
        s_.session.setStatusMessage("Clash", "A run is in progress");
        return false;
    }
    if (worker_.joinable()) worker_.join();
    busy_ = true;
    cancel_ = std::make_unique<ifcclash::CancelToken>();
    if (run_button_) run_button_->setEnabled(false);
    if (stop_button_) stop_button_->setEnabled(true);
    s_.session.beginProgress(label);
    s_.session.setStatusMessage("Clash", label + "…");
    std::vector<std::filesystem::path> paths;
    for (const QString& path : run_paths_) paths.emplace_back(path.toStdString());
    auto engine = engine_;
    worker_ = std::thread([this, paths, engine, work]() mutable {
        QString error;
        try {
            if (!engine) {
                ifcclash::SessionOptions options;
                options.triangle_source = "auto";
                engine = std::make_shared<ifcclash::Session>(paths, options);
            }
            work(*engine, engine->file_count());
        } catch (const ifcclash::Cancelled&) {
        } catch (const std::exception& e) {
            error = QString::fromUtf8(e.what());
        } catch (...) {
            error = QStringLiteral("unknown error");
        }
        const bool cancelled = cancel_ && cancel_->cancelled();
        QMetaObject::invokeMethod(
            this,
            [this, engine, cancelled, error]() {
                engine_ = engine;
                onRunFinished(cancelled, error);
            },
            Qt::QueuedConnection);
    });
    return true;
}

void ClashView::run(const std::vector<clash::RuleNode>& nodes) {
    if (busy_) {
        s_.session.setStatusMessage("Clash", "A run is in progress");
        return;
    }
    if (nodes.empty() || clash::countRules(nodes) == 0) {
        services::popup::information(host_, "There is no rule to run: add rules from the Rule Library.");
        return;
    }
    run_messages_.clear();
    QString error;
    if (!prepareModels(&error)) {
        services::popup::information(host_, error);
        return;
    }
    // The run works on a copy: the tree stays editable meanwhile.
    document_.refreshLinks();
    std::vector<clash::RuleNode> copy;
    for (const clash::RuleNode& node : nodes) {
        if (const auto* f = std::get_if<std::shared_ptr<clash::RuleFolder>>(&node)) copy.push_back(clash::json::nodeFromJson(clash::json::toJson(**f)));
        else copy.push_back(clash::json::cloneRule(*std::get<std::shared_ptr<clash::RuleCheck>>(node)));
    }
    pending_rows_.clear();
    result_texts_.clear();
    rule_panel_->tree()->setResultTexts({});
    startWork("Clash rules", [this, copy](ifcclash::Session& session, size_t files) {
        clash::Orchestrator::Callbacks callbacks;
        callbacks.finished = [this](const clash::RuleOutcome& outcome) {
            QMetaObject::invokeMethod(this, [this, outcome]() { onOutcome(outcome); }, Qt::QueuedConnection);
        };
        callbacks.progress = [this](int done, int total) {
            QMetaObject::invokeMethod(this, [this, done, total]() { s_.session.setProgress(total > 0 ? done * 100 / total : 100); },
                                      Qt::QueuedConnection);
        };
        callbacks.started = [this](const QString&, const QString& label) {
            QMetaObject::invokeMethod(this, [this, label]() { s_.session.setStatusMessage("Clash", "Running " + label + "…"); },
                                      Qt::QueuedConnection);
        };
        clash::Orchestrator orchestrator(session, files, callbacks, *cancel_);
        orchestrator.run(copy);
    });
    setProperty("work", "run");
}

void ClashView::stop() {
    if (busy_ && cancel_) {
        cancel_->cancel();
        s_.session.setStatusMessage("Clash", "Stopping after the current rule…");
    }
}

void ClashView::onOutcome(const clash::RuleOutcome& outcome) {
    if (!outcome.skipped.isEmpty()) {
        result_texts_[outcome.node_id] = "skipped";
        run_messages_ << QString("%1: skipped, %2").arg(outcome.label.isEmpty() ? "Folder" : outcome.label, outcome.skipped);
        return;
    }
    if (!outcome.error.isEmpty()) {
        result_texts_[outcome.node_id] = outcome.error == QLatin1String("cancelled") ? "stopped" : "error";
        if (outcome.error != QLatin1String("cancelled")) run_messages_ << QString("%1: %2").arg(outcome.label, outcome.error);
        return;
    }
    std::vector<QString> group_of(outcome.results.size());
    for (const clash::ResultGroup& group : outcome.groups) {
        for (int index : group.results) {
            if (index >= 0 && index < static_cast<int>(group_of.size()) && group_of[static_cast<size_t>(index)].isEmpty()) {
                group_of[static_cast<size_t>(index)] = group.label;
            }
        }
    }
    int excepted = 0;
    for (size_t i = 0; i < outcome.results.size(); ++i) {
        if (!outcome.results[i].status) ++excepted;
        pending_rows_.push_back(rowOf(outcome, outcome.results[i], group_of[i]));
    }
    QString text = QString("%1 result%2").arg(outcome.results.size()).arg(outcome.results.size() == 1 ? "" : "s");
    if (excepted > 0) text += QString(" (%1 %2)").arg(excepted).arg(outcome.groups.empty() || outcome.results.front().complex ? "failing" : "excepted");
    if (!outcome.results.empty() && outcome.results.front().complex) {
        text = QString("%1 group%2, %3 failing").arg(outcome.results.size()).arg(outcome.results.size() == 1 ? "" : "s").arg(excepted);
    }
    result_texts_[outcome.node_id] = text;
    for (const QString& message : outcome.messages) run_messages_ << QString("%1: %2").arg(outcome.label, message);
}

void ClashView::onRunFinished(bool cancelled, const QString& error) {
    if (worker_.joinable()) worker_.join();
    busy_ = false;
    if (run_button_) run_button_->setEnabled(true);
    if (stop_button_) stop_button_->setEnabled(false);
    s_.session.endProgress();
    const QString work = property("work").toString();
    setProperty("work", QVariant());
    if (!error.isEmpty()) {
        services::popup::information(host_, "The clash engine failed: " + error);
        s_.session.setStatusMessage("Clash", "Failed");
        return;
    }
    if (work != QLatin1String("run")) return;
    clearOverlay();
    results_->setResults(std::move(pending_rows_));
    pending_rows_.clear();
    info_->showRow(nullptr);
    rule_panel_->tree()->setResultTexts(result_texts_);
    applyColours();
    const size_t count = results_->rows().size();
    QString summary = QString("%1 result(s)%2").arg(count).arg(cancelled ? " (stopped)" : "");
    // The elements the engine could not test, the folders left out, the
    // failed rules: above the results, not in a modal box.
    results_->setMessages(run_messages_);
    if (!run_messages_.isEmpty()) summary += QString(", %1 warning(s) in the Result panel").arg(run_messages_.size());
    s_.session.setStatusMessage("Clash", summary);
    // The Result panel shows them, even with no result.
    if (count > 0 || !run_messages_.isEmpty()) emit showResultsRequested();
}

std::vector<std::uint32_t> ClashView::objectsOf(const clash::Elements& keys) const {
    std::vector<std::uint32_t> ids;
    for (const auto& key : keys) {
        if (key.file_index >= run_models_.size()) continue;
        const auto& elements = elements_.value(run_models_[key.file_index]);
        const auto found = elements.find(key.entity_id);
        if (found != elements.end() && found->object_id != 0) ids.push_back(found->object_id);
    }
    return ids;
}

ResultRow ClashView::rowOf(const clash::RuleOutcome& outcome, const clash::ClashResult& result, const QString& group) const {
    ResultRow row;
    row.node_id = outcome.node_id;
    row.rule_label = outcome.label;
    row.rule_type = outcome.rule_type;
    row.status = result.status;
    row.complex = result.complex;
    row.criticity = result.criticity;
    row.actor = result.actor;
    row.group = group;
    row.record = result.record;
    if (result.record) row.measure = measureText(*result.record);
    auto* loader = s_.session.loader();
    const auto describe = [&](const clash::ElementKey& key, QString& name, QString& guid, QString& cls, QString& model,
                              std::uint32_t& session_model, int& ifc_id) {
        if (key.file_index >= run_models_.size()) return;
        session_model = run_models_[key.file_index];
        ifc_id = key.entity_id;
        guid = QString::fromStdString(key.global_id);
        model = loader ? loader->displayName(session_model) : QString();
        const auto elements = elements_.value(session_model);
        const auto found = elements.find(key.entity_id);
        if (found != elements.end()) {
            name = found->name;
            cls = found->type;
        }
        if (name.isEmpty()) name = QString("#%1 %2").arg(key.entity_id).arg(guid);
    };
    if (result.complex) {
        row.source_objects = objectsOf(result.sources);
        row.target_objects = objectsOf(result.targets);
        row.source_name = QString("%1 — %2 source(s)").arg(group.isEmpty() ? QStringLiteral("Group") : group).arg(result.sources.size());
        row.target_name = QString("%1 target(s)").arg(result.targets.size());
        if (!result.sources.empty()) {
            QString n, g, c;
            describe(result.sources.front(), n, g, c, row.source_model_name, row.source_model, row.source_ifc_id);
        }
        row.measure = result.status ? "passes" : "fails";
        return row;
    }
    describe(result.source, row.source_name, row.source_guid, row.source_class, row.source_model_name, row.source_model,
             row.source_ifc_id);
    row.source_objects = objectsOf({result.source});
    if (result.target) {
        describe(*result.target, row.target_name, row.target_guid, row.target_class, row.target_model_name, row.target_model,
                 row.target_ifc_id);
        row.target_objects = objectsOf({*result.target});
    }
    return row;
}

void ClashView::showSelection(const clash::Select& select, const QString& title) {
    if (select.empty()) {
        s_.session.setStatusMessage("Clash", title + ": the slot is empty");
        return;
    }
    run_messages_.clear();
    QString error;
    if (!prepareModels(&error)) {
        services::popup::information(host_, error);
        return;
    }
    document_.refreshLinks();
    const clash::Select copy = clash::json::selectFromJson(clash::json::toJson(select));
    startWork("Selection", [this, copy, title](ifcclash::Session& session, size_t files) {
        clash::Orchestrator orchestrator(session, files, {}, *cancel_);
        const clash::Elements keys = orchestrator.select(copy);
        QMetaObject::invokeMethod(this, [this, keys, title]() {
            const auto ids = objectsOf(keys);
            if (!ids.empty()) {
                s_.pipeline.run(ids, services::ActionPipeline::Action::Isolate);
                s_.selection.setSelection(ids, services::SelectionSource::Service, services::SelectionMode::Replace);
            }
            s_.session.setStatusMessage("Clash", QString("%1: %2 element(s), %3 shown").arg(title).arg(keys.size()).arg(ids.size()));
        }, Qt::QueuedConnection);
    });
    setProperty("work", "selection");
}

// --- display -------------------------------------------------------------------------------------

void ClashView::applyColours() {
    clearColours();
    std::vector<std::uint32_t> sources, targets, passing;
    for (const ResultRow& row : results_->rows()) {
        if (row.complex) {
            auto& set = row.status ? passing : sources;
            set.insert(set.end(), row.source_objects.begin(), row.source_objects.end());
            set.insert(set.end(), row.target_objects.begin(), row.target_objects.end());
            continue;
        }
        if (!row.status) continue; // excepted
        sources.insert(sources.end(), row.source_objects.begin(), row.source_objects.end());
        targets.insert(targets.end(), row.target_objects.begin(), row.target_objects.end());
    }
    if (sources.empty() && targets.empty() && passing.empty()) return;
    const bool was_suspended = s_.undo.isSuspended();
    s_.undo.record();
    s_.undo.setSuspended(true);
    s_.appearance.setLayerColour(passing, kPassColour, Layer::ClashRun);
    s_.appearance.setLayerColour(targets, kTargetColour, Layer::ClashRun);
    s_.appearance.setLayerColour(sources, kSourceColour, Layer::ClashRun); // a source wins over a target
    s_.undo.setSuspended(was_suspended);
}

void ClashView::clearColours() {
    s_.appearance.clearLayer(Layer::ClashRun);
}

void ClashView::clearOverlay() {
    // Only our own lines: the slot is shared with the measure tools.
    if (!overlay_drawn_) return;
    s_.viewport.setOverlayLines({});
    overlay_drawn_ = false;
}

void ClashView::drawOverlay(int index) {
    if (index < 0 || index >= static_cast<int>(results_->rows().size()) || !overlay_.any()) {
        clearOverlay();
        return;
    }
    const ResultRow& row = results_->rows()[static_cast<size_t>(index)];
    if (!row.record) {
        clearOverlay();
        return;
    }
    Transform t;
    Eigen::Matrix4d m;
    if (s_.viewport.modelToWorld(row.source_model, m)) t.m = m;
    const ifcclash::ResultRecord& r = *row.record;
    auto points = lineGroup(QColor("#00B4D8"), 2.5f);
    for (const auto& p : r.points) t.segment(points.world_xyz, p.source, p.target);
    auto zones = lineGroup(QColor("#CC33FF"), 2.0f, 12.0f);
    auto boxes = lineGroup(QColor("#FFD91A"), 1.5f);
    for (const auto* list : {&r.source_obbs, &r.target_obbs}) {
        for (const auto& obb : *list) obbEdges(t, obb, obb.label && *obb.label == "zone" ? zones.world_xyz : boxes.world_xyz);
    }
    auto axes = lineGroup(QColor("#3CB44B"), 2.0f);
    for (const auto& axis : r.axes) {
        ifcclash::Point3 a, b;
        for (size_t c = 0; c < 3; ++c) {
            a[c] = axis.center[c] - 0.5 * axis.length * axis.direction[c];
            b[c] = axis.center[c] + 0.5 * axis.length * axis.direction[c];
        }
        t.segment(axes.world_xyz, a, b);
    }
    auto faces = lineGroup(QColor("#F08A24"), 1.2f);
    for (const auto& pair : r.faces) {
        for (const ifcclash::FaceData* face : {&pair.source, &pair.target}) {
            for (const auto& tri : face->triangles) {
                t.segment(faces.world_xyz, tri[0], tri[1]);
                t.segment(faces.world_xyz, tri[1], tri[2]);
                t.segment(faces.world_xyz, tri[2], tri[0]);
            }
        }
    }
    auto hits = lineGroup(QColor("#3CB44B"), 1.0f);
    auto blocked = lineGroup(QColor("#E6194B"), 1.0f);
    if (r.raycast) {
        for (const auto& [a, b] : r.raycast->hit_segments) t.segment(hits.world_xyz, a, b);
        for (const auto& [a, b] : r.raycast->blocked_segments) t.segment(blocked.world_xyz, a, b);
    }
    std::vector<OverlayRenderer::LineGroup> groups;
    for (const auto& [group, on] : std::vector<std::pair<OverlayRenderer::LineGroup*, bool>>{
             {&zones, overlay_.zones}, {&boxes, overlay_.obbs}, {&faces, overlay_.faces}, {&axes, overlay_.axes},
             {&hits, overlay_.rays}, {&blocked, overlay_.rays}, {&points, overlay_.points}}) {
        if (on && !group->world_xyz.empty()) groups.push_back(std::move(*group));
    }
    if (groups.empty()) {
        clearOverlay();
        return;
    }
    s_.viewport.setOverlayLines(groups);
    overlay_drawn_ = true;
}

void ClashView::isolateRows(const std::vector<int>& rows) {
    std::vector<std::uint32_t> ids;
    for (int row : rows) {
        if (row < 0 || row >= static_cast<int>(results_->rows().size())) continue;
        const auto objects = results_->rows()[static_cast<size_t>(row)].objects();
        ids.insert(ids.end(), objects.begin(), objects.end());
    }
    if (ids.empty()) {
        s_.session.setStatusMessage("Clash", "The result has no element with geometry in the viewer");
        return;
    }
    std::vector<std::uint32_t> shown = ids;
    if (always_context_) shown.insert(shown.end(), context_.begin(), context_.end());
    s_.pipeline.run(shown, services::ActionPipeline::Action::Isolate);
    s_.selection.setSelection(ids, services::SelectionSource::Table, services::SelectionMode::Replace);
    modules::viewport::commands::viewSelected(s_.viewport);
    if (rows.size() == 1) drawOverlay(rows.front());
}

// --- files -----------------------------------------------------------------------------------------

bool ClashView::confirmDiscard() {
    if (!document_.isOpen() || !document_.isModified() || document_.file().contains.empty()) return true;
    const auto answer = QMessageBox::question(host_, "Rule file", "The rule file is modified. Save it?",
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Save) {
        saveRules(false);
        return !document_.isModified();
    }
    return true;
}

void ClashView::saveRules(bool ask_path) {
    if (!document_.isOpen()) return;
    QString path = document_.path();
    if (ask_path || path.isEmpty()) {
        QSettings settings;
        path = QFileDialog::getSaveFileName(host_, "Save the rule file", settings.value(kLastDirectoryKey).toString(),
                                            "Rule files (*.json);;All files (*)");
        if (path.isEmpty()) return;
        if (QFileInfo(path).suffix().isEmpty()) path += ".json";
        settings.setValue(kLastDirectoryKey, QFileInfo(path).absolutePath());
    }
    document_.refreshLinks();
    clash::RuleFile& file = document_.file();
    file.list_ifc_path.clear(); // not saved: the rules apply to any set of models
    file.path_to_save = path;
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(QJsonDocument(clash::json::toJson(file)).toJson(QJsonDocument::Indented)) < 0 ||
        !out.commit()) {
        services::popup::information(host_, QString("Cannot write %1.").arg(path));
        return;
    }
    document_.setPath(path);
    document_.markSaved();
    s_.session.setStatusMessage("Clash", "Rule file saved: " + QFileInfo(path).fileName());
}

void ClashView::loadRules() {
    if (!confirmDiscard()) return;
    QSettings settings;
    const QString path = QFileDialog::getOpenFileName(host_, "Open a rule file", settings.value(kLastDirectoryKey).toString(),
                                                      "Rule files (*.json);;All files (*)");
    if (path.isEmpty()) return;
    settings.setValue(kLastDirectoryKey, QFileInfo(path).absolutePath());
    QFile in(path);
    if (!in.open(QIODevice::ReadOnly)) {
        services::popup::information(host_, QString("Cannot read %1.").arg(path));
        return;
    }
    try {
        document_.setFile(clash::json::fileFromJson(clash::json::parseDocument(in.readAll())), path);
    } catch (const std::exception& e) {
        services::popup::information(host_, QString("%1 is not a rule file: %2").arg(QFileInfo(path).fileName(), e.what()));
        return;
    }
    QStringList unknown;
    for (const auto& rule : document_.treeRules()) {
        if (!clash::RuleCatalogue::instance().find(rule->type)) unknown << rule->type;
    }
    unknown.removeDuplicates();
    if (!unknown.isEmpty()) {
        services::popup::information(host_, "These rules have no triangle version in the viewer and will fail: " + unknown.join(", "));
    }
    s_.session.setStatusMessage("Clash", "Rule file loaded: " + QFileInfo(path).fileName());
}

void ClashView::saveResults() {
    if (results_->rows().empty()) {
        services::popup::information(host_, "No result to save: run the rules first.");
        return;
    }
    QSettings settings;
    QString path = QFileDialog::getSaveFileName(host_, "Save the results", settings.value(kLastDirectoryKey).toString(),
                                                "JSON files (*.json)");
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += ".json";
    const auto review = [](ReviewStatus status) {
        return status == ReviewStatus::Valid ? "Valid" : status == ReviewStatus::Invalid ? "Invalid" : "Non Determined";
    };
    const auto element = [](const QString& name, const QString& guid, const QString& cls, const QString& model, int id) {
        return QJsonObject{{"name", name}, {"global_id", guid}, {"ifc_type", cls}, {"model", model}, {"step_id", id}};
    };
    QJsonArray results;
    for (const ResultRow& row : results_->rows()) {
        QJsonObject json{{"rule", row.rule_label}, {"rule_type", row.rule_type}, {"node_id", row.node_id},
                         {"status", row.status}, {"review", review(row.review)}, {"measure", row.measure},
                         {"criticity", QJsonArray::fromStringList(row.criticity)}, {"actor", QJsonArray::fromStringList(row.actor)},
                         {"source", element(row.source_name, row.source_guid, row.source_class, row.source_model_name, row.source_ifc_id)}};
        if (!row.target_guid.isEmpty()) {
            json["target"] = element(row.target_name, row.target_guid, row.target_class, row.target_model_name, row.target_ifc_id);
        }
        if (!row.group.isEmpty()) json["group"] = row.group;
        if (row.record) json["info"] = QJsonDocument::fromJson(QByteArray::fromStdString(ifcclash::info_to_json(*row.record).dump())).object();
        results.append(json);
    }
    document_.refreshLinks();
    const QJsonObject json{{"rule_file", clash::json::toJson(document_.file())}, {"results", results}};
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(QJsonDocument(json).toJson(QJsonDocument::Indented)) < 0 || !out.commit()) {
        services::popup::information(host_, QString("Cannot write %1.").arg(path));
        return;
    }
    s_.session.setStatusMessage("Clash", QString("%1 result(s) saved").arg(results.size()));
}

void ClashView::editRule(std::shared_ptr<clash::RuleCheck> rule) {
    if (!rule) return;
    RuleEditDialog dialog(
        rule, &document_,
        RuleEditDialog::FacetContext{s_.ids, [this]() { return services::FilterRunner::activeIfcPath(s_.session); }},
        host_);
    // The preview of an IDS selection: EXPRESS ids of the active model -> object ids -> isolate.
    connect(&dialog, &RuleEditDialog::isolateRequested, this, [this](const std::vector<int>& express_ids) {
        std::uint32_t model = 0;
        services::FilterRunner::activeIfcPath(s_.session, &model);
        auto* registry = s_.session.elementRegistry();
        if (model == 0 || registry == nullptr) return;
        QHash<int, std::uint32_t> object_of;
        for (const auto& info : registry->basicElementInfoForModel(model)) object_of.insert(info.ifc_id, info.object_id);
        std::vector<std::uint32_t> ids;
        for (int id : express_ids) {
            const auto found = object_of.find(id);
            if (found != object_of.end()) ids.push_back(found.value());
        }
        s_.pipeline.run(ids, services::ActionPipeline::Action::Isolate);
    });
    // A rule edited from a cascade is written at once (its own window's OK).
    connect(&dialog, &RuleEditDialog::ruleEdited, &document_, &RuleDocument::touch);
    if (dialog.exec() != QDialog::Accepted) return;
    dialog.apply();
    document_.touch();
}

void ClashView::editFolder(std::shared_ptr<clash::RuleFolder> folder) {
    if (!folder) return;
    FolderEditDialog dialog(folder, host_);
    if (dialog.exec() != QDialog::Accepted) return;
    dialog.apply();
    document_.touch();
}

// --- ribbons -----------------------------------------------------------------------------------------

QWidget* ClashView::buildGeometryRuleRibbon() {
    auto* page = new QFrame(host_);
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);
    const auto describe = [](QToolButton* button, const QString& text) {
        button->setToolTip(QString("<b>%1</b><br>%2").arg(button->text(), text));
    };
    run_button_ = components::buttons::makeButton("Run Rule", ":/icons/intersect.svg", page);
    describe(run_button_, "Run the selected rules and folders of the Rule tree, or the whole rule file.");
    connect(run_button_, &QToolButton::clicked, this, [this]() {
        auto nodes = rule_panel_->tree()->selectedNodes();
        if (nodes.empty()) nodes = document_.file().contains;
        run(nodes);
    });
    stop_button_ = components::buttons::makeButton("Stop", ":/icons/xmark-circle.svg", page);
    stop_button_->setEnabled(false);
    describe(stop_button_, "Stop the run after the current rule.");
    connect(stop_button_, &QToolButton::clicked, this, &ClashView::stop);
    auto* show = components::buttons::makeButton("Show Results", ":/icons/check.svg", page);
    describe(show, "Show the Clash Filter workspace with the results of the last run.");
    connect(show, &QToolButton::clicked, this, &ClashView::showResultsRequested);
    auto* save = components::buttons::makeButton("Save Rules", ":/icons/floppy-disk.svg", page);
    describe(save, "Save the rule file (JSON, the format of the IfcClash_Plus command line).");
    connect(save, &QToolButton::clicked, this, [this]() { saveRules(false); });
    auto* save_as = components::buttons::makeButton("Save As", ":/icons/floppy-disk-arrow-in.svg", page);
    describe(save_as, "Save the rule file under a new name.");
    connect(save_as, &QToolButton::clicked, this, [this]() { saveRules(true); });
    auto* load = components::buttons::makeButton("Load Rules", ":/icons/folder.svg", page);
    describe(load, "Open a rule file (JSON).");
    connect(load, &QToolButton::clicked, this, &ClashView::loadRules);
    auto* colour = components::buttons::makeButton("Color Results", ":/icons/paint-bucket.svg", page);
    colour->setCheckable(true);
    colour->setChecked(true);
    describe(colour, "Colour the elements of the results: sources red, targets orange, Must groups green or red.");
    connect(colour, &QToolButton::toggled, this, [this](bool on) {
        if (on) applyColours();
        else clearColours();
    });
    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("RUN", {run_button_, stop_button_, show}, page),
        components::buttons::makeButtonGroup("RULE FILE", {save, save_as, load}, page),
        components::buttons::makeButtonGroup("COLOUR", {colour}, page),
    });
    row->addStretch(1);
    return page;
}

QWidget* ClashView::buildClashFilterRibbon() {
    auto* page = new QFrame(host_);
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);
    const auto describe = [](QToolButton* button, const QString& text) {
        button->setToolTip(QString("<b>%1</b><br>%2").arg(button->text(), text));
    };
    auto* save_context = components::buttons::makeButton("Save Context", ":/icons/floppy-disk.svg", page);
    describe(save_context, "Keep the elements shown now as the context of the results.");
    connect(save_context, &QToolButton::clicked, this, [this]() {
        context_ = s_.view.visibleIds();
        s_.session.setStatusMessage("Clash", QString("Context saved: %1 object(s)").arg(context_.size()));
    });
    auto* show_context = components::buttons::makeButton("Show Context", ":/icons/eye.svg", page);
    describe(show_context, "Show the saved context again, beside what is shown.");
    connect(show_context, &QToolButton::clicked, this, [this]() {
        if (context_.empty()) {
            s_.session.setStatusMessage("Clash", "No saved context");
            return;
        }
        s_.view.add(context_);
    });
    auto* always = components::buttons::makeButton("Always Display Context", ":/icons/cube-dots.svg", page);
    always->setCheckable(true);
    describe(always, "When a result is isolated, the saved context stays shown around it.");
    connect(always, &QToolButton::toggled, this, [this](bool on) { always_context_ = on; });
    auto* colour = components::buttons::makeButton("Color Result", ":/icons/paint-bucket.svg", page);
    describe(colour, "Colour the elements of the results again (sources red, targets orange).");
    connect(colour, &QToolButton::clicked, this, &ClashView::applyColours);

    auto* reset = components::buttons::makeButton("Show All Results", ":/icons/refresh-double.svg", page);
    describe(reset, "Clear the filters and the grouping of the Result panel.");
    connect(reset, &QToolButton::clicked, results_, &ResultPanel::clearFilters);
    auto* undetermined = components::buttons::makeButton("Only Non Determined", ":/icons/filter.svg", page);
    undetermined->setCheckable(true);
    describe(undetermined, "Show only the results not reviewed yet.");
    connect(undetermined, &QToolButton::toggled, results_, &ResultPanel::setFilterOnlyNonDetermined);
    auto* excepted = components::buttons::makeButton("Hide Excepted", ":/icons/eye-closed.svg", page);
    excepted->setCheckable(true);
    describe(excepted, "Hide the results an exception invalidated.");
    connect(excepted, &QToolButton::toggled, results_, &ResultPanel::setHideExcepted);
    auto* group = components::buttons::makeButton("Group by", ":/icons/cube.svg", page);
    group->setPopupMode(QToolButton::InstantPopup);
    describe(group, "Group the results by source, target, rule, criticity or the rule's own grouping.");
    auto* menu = new QMenu(group);
    auto* modes = new QActionGroup(menu);
    for (const auto& [label, mode] : std::vector<std::pair<QString, ResultPanel::GroupMode>>{
             {"No grouping", ResultPanel::GroupMode::Flat},
             {"Source", ResultPanel::GroupMode::BySource},
             {"Target", ResultPanel::GroupMode::ByTarget},
             {"Rule", ResultPanel::GroupMode::ByRule},
             {"Criticity", ResultPanel::GroupMode::ByCriticity},
             {"Rule grouping", ResultPanel::GroupMode::ByRuleGroup}}) {
        QAction* action = menu->addAction(label, this, [this, mode = mode]() { results_->setGroupMode(mode); });
        action->setCheckable(true);
        action->setChecked(mode == ResultPanel::GroupMode::Flat);
        modes->addAction(action);
    }
    group->setMenu(menu);
    // The geometry of the current result (one row selected), each kind on / off.
    QList<QToolButton*> geometry;
    for (const auto& [text, icon, flag, tip] : std::vector<std::tuple<QString, QString, bool OverlayKinds::*, QString>>{
             {"Points", ":/icons/select-point3d.svg", &OverlayKinds::points,
              "The closest points (or the contact points) between the two elements, in cyan."},
             {"Zones", ":/icons/cube-scan.svg", &OverlayKinds::zones,
              "The zones the rule checked (above, below, around the element…), dashed magenta boxes."},
             {"OBB", ":/icons/box-3d-center.svg", &OverlayKinds::obbs,
              "The oriented bounding boxes of the elements the rule used, in yellow."},
             {"Axes", ":/icons/select-edge3d.svg", &OverlayKinds::axes,
              "The axes of the elements (main direction, length), in green."},
             {"Faces", ":/icons/select-face3d.svg", &OverlayKinds::faces,
              "The faces in contact or facing each other, in orange."},
             {"Rays", ":/icons/perspective-view.svg", &OverlayKinds::rays,
              "The rays of a visibility / free space rule: free in green, blocked in red."}}) {
        auto* button = components::buttons::makeButton(text, icon, page);
        button->setCheckable(true);
        describe(button, tip + "<br><i>Drawn for the selected result, over the 3D view; the colours and the "
                               "visibility of the results do not change.</i>");
        connect(button, &QToolButton::toggled, this, [this, flag = flag](bool on) {
            overlay_.*flag = on;
            const auto rows = results_->selectedRows();
            drawOverlay(rows.size() == 1 ? rows.front() : -1);
        });
        geometry.push_back(button);
    }
    auto* save = components::buttons::makeButton("Save Results", ":/icons/download-square.svg", page);
    describe(save, "Save the results of the last run (and the rules) to a JSON file, with the reviews.");
    connect(save, &QToolButton::clicked, this, &ClashView::saveResults);
    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("CONTEXT VIEW", {save_context, show_context, always, colour}, page),
        components::buttons::makeButtonGroup("REGROUP RESULT", {reset, undetermined, excepted, group}, page),
        components::buttons::makeButtonGroup("RESULT GEOMETRY", geometry, page),
        components::buttons::makeButtonGroup("EXPORT", {save}, page),
    });
    row->addStretch(1);
    return page;
}

} // namespace bonsaiviewer::modules::rule
