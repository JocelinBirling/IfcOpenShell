// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                           *
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

#include "Panel.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../components/SvgIcon.h"
#include "../../services/ActionPipeline.h"
#include "../../services/ContextMenuBuilder.h"
#include "../../services/SelectionService.h"
#include "../../../ifcviewer/Federation.h"
#include "../../../ifcviewer/SceneLoader.h"
#include "../../../ifcviewer/ViewportWindow.h"
#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"

#include "element.h"  // helpers: get_aggregate, get_container, get_scalar_attributes, get_string_attribute

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <unordered_map>
#include <unordered_set>

namespace bonsaiviewer::modules::unified_tree {

namespace unified = bonsaiviewer::services::unified;

namespace {

constexpr int kPlaceholder = -1;  // the child of a folder not expanded yet

std::string displayName(const express::base& element) {
    if (auto name = get_string_attribute(element, "Name"); name && !name->empty()) return *name;
    if (auto long_name = get_string_attribute(element, "LongName"); long_name && !long_name->empty()) return *long_name;
    return {};
}

// The nearest IfcBuildingStorey at or above `level`, walking the aggregation
// up (a space sits in a storey). 0 when none.
int storeyAbove(express::base level) {
    for (int guard = 0; level && guard < 64; ++guard) {
        if (level.declaration().is("IfcBuildingStorey")) return static_cast<int>(level.id());
        level = get_aggregate(level);
    }
    return 0;
}

std::optional<double> elevationAttribute(const express::base& storey) {
    for (const auto& [name, value] : get_scalar_attributes(storey)) {
        if (name != "Elevation") continue;
        try {
            return std::stod(value);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

QString iconOf(unified::Node::Kind kind) {
    switch (kind) {
    case unified::Node::Kind::Site: return ":/icons/frame-alt.svg";
    case unified::Node::Kind::Building: return ":/icons/city.svg";
    case unified::Node::Kind::Level: return ":/icons/planimetry.svg";
    case unified::Node::Kind::Room: return ":/icons/square3d-from-center.svg";
    case unified::Node::Kind::NoRoom:
    case unified::Node::Kind::NoLevel: return ":/icons/folder.svg";
    case unified::Node::Kind::Element: return ":/icons/cube.svg";
    case unified::Node::Kind::Diagnostics: return ":/icons/xmark-circle.svg";
    case unified::Node::Kind::Message: return QString();
    }
    return QString();
}

} // namespace

UnifiedTreeView::UnifiedTreeView(UnifiedTreePanel* panel,
                                 bonsaiviewer::SessionState& session,
                                 ViewportWindow& viewport,
                                 bonsaiviewer::services::SelectionService& selection,
                                 bonsaiviewer::services::ActionPipeline& actions,
                                 QObject* parent)
    : QObject(parent), panel_(panel), session_(session), viewport_(viewport),
      selection_(selection), actions_(actions)
{
    QTreeWidget* tree = panel_->tree();
    connect(panel_, &UnifiedTreePanel::settingsRequested, this, [this]() { openSettings(); });
    connect(panel_, &UnifiedTreePanel::runRequested, this, [this]() { run(); });
    connect(tree, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem* item) { fillChildren(item); });
    // Click selects the node and everything below; several nodes with Ctrl / Shift.
    connect(tree, &QTreeWidget::itemSelectionChanged, this, [this, tree]() {
        std::vector<std::uint32_t> ids;
        for (QTreeWidgetItem* item : tree->selectedItems()) {
            const auto below = unified::objectIdsBelow(result_, nodeOf(item));
            ids.insert(ids.end(), below.begin(), below.end());
        }
        if (!ids.empty()) selection_.setSelection(ids, services::SelectionSource::Tree);
    });
    // Double-click isolates.
    connect(tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        const auto ids = unified::objectIdsBelow(result_, nodeOf(item));
        if (!ids.empty()) actions_.run(ids, services::ActionPipeline::Action::Isolate);
    });
    connect(tree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) { showMenu(pos); });
    // The index lives in memory for the current project only.
    connect(&session_, &bonsaiviewer::SessionState::projectOpened, this,
            [this](const QString&) { clear("Not computed. Choose the settings, then Run."); });
    connect(&session_, &bonsaiviewer::SessionState::projectReset, this,
            [this]() { clear("Not computed. Choose the settings, then Run."); });
}

QString UnifiedTreeView::modelName(std::uint32_t session_model_id) const {
    const QString model_id = session_.modelIdForSessionModelId(session_model_id);
    if (const auto* model = session_.federation()->findById(model_id); model && !model->display_name.isEmpty()) {
        return model->display_name;
    }
    return session_.loader() ? session_.loader()->displayName(session_model_id) : QString();
}

std::uint32_t UnifiedTreeView::defaultReferenceModel() const {
    std::uint32_t best = 0;
    int best_count = -1;
    for (std::uint32_t model : session_.sessionModelIds()) {
        int count = 0;
        for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(model)) {
            if (info.type == QLatin1String("IfcSpace")) ++count;
        }
        if (count > best_count) {
            best = model;
            best_count = count;
        }
    }
    return best;
}

void UnifiedTreeView::openSettings() {
    const QList<std::uint32_t> models = session_.sessionModelIds();
    if (models.isEmpty()) {
        panel_->setStatus("No model is open.");
        return;
    }
    if (!settings_chosen_ || !models.contains(settings_.levels_model) || !models.contains(settings_.rooms_model)) {
        const std::uint32_t reference = defaultReferenceModel();
        settings_.levels_model = reference;
        settings_.rooms_model = reference;
    }

    QDialog dialog(panel_);
    dialog.setWindowTitle("Unified Tree settings");
    auto* form = new QFormLayout(&dialog);
    auto* levels = new QComboBox(&dialog);
    auto* rooms = new QComboBox(&dialog);
    for (std::uint32_t model : models) {
        levels->addItem(modelName(model), model);
        rooms->addItem(modelName(model), model);
    }
    levels->setCurrentIndex(levels->findData(settings_.levels_model));
    rooms->setCurrentIndex(rooms->findData(settings_.rooms_model));
    auto* tolerance = new QDoubleSpinBox(&dialog);
    tolerance->setRange(0.01, 1.00);
    tolerance->setSingleStep(0.01);
    tolerance->setDecimals(2);
    tolerance->setSuffix(" m");
    tolerance->setValue(settings_.tolerance);
    auto* merge_sites = new QCheckBox("Merge all sites into one", &dialog);
    merge_sites->setChecked(settings_.merge_sites);
    auto* merge_buildings = new QCheckBox("Merge all buildings into one", &dialog);
    merge_buildings->setChecked(settings_.merge_buildings);
    auto* merge_names = new QCheckBox("Merge levels with the same name", &dialog);
    merge_names->setChecked(settings_.merge_same_name);
    auto* one_level = new QCheckBox("One level per element", &dialog);
    one_level->setToolTip("An element overlapping rooms of several levels is kept on one level only:\n"
                          "the level of its IFC storey, else the level it overlaps the most.");
    one_level->setChecked(settings_.one_level_per_element);
    form->addRow("Levels reference model", levels);
    form->addRow("Rooms reference model", rooms);
    form->addRow("Level tolerance", tolerance);
    form->addRow(merge_sites);
    form->addRow(merge_buildings);
    form->addRow(merge_names);
    form->addRow(one_level);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);
    if (dialog.exec() != QDialog::Accepted) return;

    settings_.levels_model = levels->currentData().toUInt();
    settings_.rooms_model = rooms->currentData().toUInt();
    settings_.tolerance = tolerance->value();
    settings_.merge_sites = merge_sites->isChecked();
    settings_.merge_buildings = merge_buildings->isChecked();
    settings_.merge_same_name = merge_names->isChecked();
    settings_.one_level_per_element = one_level->isChecked();
    settings_chosen_ = true;
    panel_->setStatus("Settings changed. Run to compute.");
}

bool UnifiedTreeView::collect(unified::Input& input, QString& error) const {
    auto* loader = session_.loader();
    auto* registry = session_.elementRegistry();
    if (loader == nullptr || registry == nullptr) {
        error = "No model is open.";
        return false;
    }
    input.settings = settings_;
    for (std::uint32_t model : session_.sessionModelIds()) {
        ifcopenshell::file* file = loader->ifcFile(model);
        if (file == nullptr) {
            error = QString("The IFC data of %1 is not open yet: wait for it, then Run again.").arg(modelName(model));
            return false;
        }
        const std::string model_name = modelName(model).toStdString();
        const double unit = computeModelGeoref(file).units.project_length_to_meters;
        Eigen::Matrix4d to_world = Eigen::Matrix4d::Identity();
        viewport_.modelToWorld(model, to_world);

        try {
            // Storeys, with their world elevation and their site / building.
            for (const express::base& storey : file->instances_by_type("IfcBuildingStorey")) {
                unified::StoreyIn in;
                in.model = model;
                in.model_name = model_name;
                in.ifc_id = static_cast<int>(storey.id());
                in.name = displayName(storey);
                if (const auto elevation = elevationAttribute(storey)) {
                    in.elevation = (to_world * Eigen::Vector4d(0.0, 0.0, *elevation * unit, 1.0))(2);
                }
                express::base level = get_aggregate(storey);
                for (int guard = 0; level && guard < 64; ++guard) {
                    if (in.building_ifc == 0 && level.declaration().is("IfcBuilding")) {
                        in.building_ifc = static_cast<int>(level.id());
                        in.building_name = displayName(level);
                    } else if (in.site_ifc == 0 && level.declaration().is("IfcSite")) {
                        in.site_ifc = static_cast<int>(level.id());
                        in.site_name = displayName(level);
                    }
                    level = get_aggregate(level);
                }
                input.storeys.push_back(std::move(in));
            }

            // World boxes of every object of the model, in one pass.
            std::unordered_map<std::uint32_t, std::pair<Eigen::Vector3f, Eigen::Vector3f>> boxes;
            viewport_.collectModelObjectAabbs(model, boxes);
            auto toBox = [](const std::pair<Eigen::Vector3f, Eigen::Vector3f>& aabb) {
                unified::Box box;
                for (int axis = 0; axis < 3; ++axis) {
                    box.mn[axis] = aabb.first[axis];
                    box.mx[axis] = aabb.second[axis];
                }
                return box;
            };
            const auto elements = registry->basicElementInfoForModel(model);

            // Real shapes in plan, for the rooms, the walls and the slabs.
            auto isWall = [](const QString& type) {
                return type.startsWith(QLatin1String("IfcWall")) || type.startsWith(QLatin1String("IfcCurtainWall"))
                    || type.startsWith(QLatin1String("IfcColumn"));
            };
            std::unordered_set<std::uint32_t> shaped;
            for (const auto& info : elements) {
                if (info.type.startsWith(QLatin1String("IfcSlab")) || isWall(info.type)
                    || (model == settings_.rooms_model && info.type == QLatin1String("IfcSpace"))) {
                    shaped.insert(info.object_id);
                }
            }
            std::unordered_map<std::uint32_t, std::vector<float>> footprints;
            viewport_.collectModelObjectTrianglesXY(model, shaped, footprints);
            auto footprintOf = [&footprints](std::uint32_t object_id) {
                const auto found = footprints.find(object_id);
                return found == footprints.end() ? std::vector<float>() : std::move(found->second);
            };

            // Rooms: the spaces of the rooms reference model.
            if (model == settings_.rooms_model) {
                std::unordered_map<int, std::uint32_t> object_by_ifc;
                for (const auto& info : elements) object_by_ifc.emplace(info.ifc_id, info.object_id);
                for (const express::base& space : file->instances_by_type("IfcSpace")) {
                    unified::RoomIn room;
                    room.ifc_id = static_cast<int>(space.id());
                    room.name = displayName(space);
                    const auto object = object_by_ifc.find(room.ifc_id);
                    if (object != object_by_ifc.end()) {
                        room.object_id = object->second;
                        const auto box = boxes.find(room.object_id);
                        if (box != boxes.end()) room.box = toBox(box->second);
                        else room.object_id = 0;
                        room.footprint = footprintOf(object->second);
                    }
                    express::base start = get_aggregate(space);
                    if (!start) start = get_container(space);
                    room.storey_ifc = storeyAbove(start);
                    input.rooms.push_back(std::move(room));
                }
            }

            // Elements with geometry, with the storey that contains them.
            std::unordered_map<int, int> storey_of_container;
            for (const auto& info : elements) {
                const auto box = boxes.find(info.object_id);
                if (box == boxes.end()) continue;
                unified::ElementIn element;
                element.object_id = info.object_id;
                element.model = model;
                element.ifc_id = info.ifc_id;
                element.label = (info.name.isEmpty() ? info.guid : info.name).toStdString();
                element.is_slab = info.type.startsWith(QLatin1String("IfcSlab"));
                element.is_wall = isWall(info.type);
                element.box = toBox(box->second);
                if (element.is_slab || element.is_wall) element.footprint = footprintOf(info.object_id);
                if (info.ifc_id != 0) {
                    const express::base entity = file->instance_by_id(info.ifc_id);
                    if (const express::base container = get_container(entity)) {
                        const int container_id = static_cast<int>(container.id());
                        auto cached = storey_of_container.find(container_id);
                        if (cached == storey_of_container.end()) {
                            cached = storey_of_container.emplace(container_id, storeyAbove(container)).first;
                        }
                        element.storey_ifc = cached->second;
                    }
                }
                input.elements.push_back(std::move(element));
            }
        } catch (const std::exception& e) {
            error = QString("%1 cannot be read: %2").arg(modelName(model), QString::fromUtf8(e.what()));
            return false;
        }
    }
    return true;
}

void UnifiedTreeView::clear(const QString& status) {
    result_ = {};
    panel_->tree()->clear();
    panel_->setStatus(status);
}

void UnifiedTreeView::run() {
    if (session_.sessionModelIds().isEmpty()) {
        clear("No model is open.");
        return;
    }
    if (!settings_chosen_) {
        const std::uint32_t reference = defaultReferenceModel();
        settings_.levels_model = reference;
        settings_.rooms_model = reference;
        settings_chosen_ = true;
    }
    panel_->setRunning(true);
    panel_->setStatus("Computing…");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    unified::Input input;
    QString error;
    if (collect(input, error)) {
        result_ = unified::build(input);
        if (!result_.ok) error = QString::fromStdString(result_.error);
    }
    QApplication::restoreOverrideCursor();
    panel_->setRunning(false);

    if (!error.isEmpty()) {
        clear("Not computed: " + error);
        session_.setStatusMessage("Unified Tree", error);
        return;
    }
    populate();
    int levels = 0, rooms = 0;
    for (const auto& node : result_.nodes) {
        if (node.kind == unified::Node::Kind::Level) ++levels;
        if (node.kind == unified::Node::Kind::Room) ++rooms;
    }
    const QString summary = QString("%1 levels, %2 rooms, %3 elements, %4 diagnostics.")
                                .arg(levels)
                                .arg(rooms)
                                .arg(input.elements.size())
                                .arg(result_.diagnostics.size());
    panel_->setStatus(summary);
    session_.setStatusMessage("Unified Tree", summary);
}

int UnifiedTreeView::nodeOf(const QTreeWidgetItem* item) const {
    return item ? item->data(0, Qt::UserRole).toInt() : kPlaceholder;
}

QTreeWidgetItem* UnifiedTreeView::makeItem(int index) const {
    const unified::Node& node = result_.nodes[index];
    QString label = QString::fromStdString(node.label);
    const bool folder = node.kind != unified::Node::Kind::Element && node.kind != unified::Node::Kind::Message;
    if (folder) label += QString(" (%1)").arg(node.count);
    if (node.kind == unified::Node::Kind::Element && node.multiplicity > 1) {
        label += QString::fromUtf8(" ×%1").arg(node.multiplicity);
    }
    auto* item = new QTreeWidgetItem({label, QString()});
    item->setData(0, Qt::UserRole, index);
    item->setSizeHint(0, QSize(0, 22));
    const QString icon = iconOf(node.kind);
    if (!icon.isEmpty()) item->setIcon(0, components::icons::makeSvgIcon(icon));
    // Deduced: an icon in the second column; the tooltip gives the reason.
    if (node.reason != unified::Reason::None) {
        item->setIcon(1, components::icons::makeSvgIcon(":/icons/intersect.svg"));
        const QString why = QString::fromStdString(node.reason_text);
        item->setToolTip(0, why);
        item->setToolTip(1, why);
    } else if (node.kind == unified::Node::Kind::Message) {
        item->setToolTip(0, label);
    }
    if (!node.children.empty()) {
        auto* placeholder = new QTreeWidgetItem(item);
        placeholder->setData(0, Qt::UserRole, kPlaceholder);
    }
    return item;
}

void UnifiedTreeView::fillChildren(QTreeWidgetItem* item) {
    if (item->childCount() != 1 || nodeOf(item->child(0)) != kPlaceholder) return;
    delete item->takeChild(0);
    QList<QTreeWidgetItem*> children;
    for (int child : result_.nodes[nodeOf(item)].children) children.append(makeItem(child));
    item->addChildren(children);
}

void UnifiedTreeView::populate() {
    QTreeWidget* tree = panel_->tree();
    tree->clear();
    for (int root : result_.roots) tree->addTopLevelItem(makeItem(root));
    // Sites and buildings open; the levels stay closed.
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* site = tree->topLevelItem(i);
        if (result_.nodes[nodeOf(site)].kind != unified::Node::Kind::Site) continue;
        site->setExpanded(true);
        for (int j = 0; j < site->childCount(); ++j) site->child(j)->setExpanded(true);
    }
}

void UnifiedTreeView::showMenu(const QPoint& pos) {
    QTreeWidget* tree = panel_->tree();
    QTreeWidgetItem* item = tree->itemAt(pos);
    const int index = nodeOf(item);
    if (index < 0) return;
    const unified::Node node = result_.nodes[index];
    services::ContextMenuBuilder menu;
    menu.addItem("isolate", "Isolate", [this, index] {
        const auto ids = unified::objectIdsBelow(result_, index);
        if (!ids.empty()) actions_.run(ids, services::ActionPipeline::Action::Isolate);
    });
    menu.addItem("select", "Select", [this, index] {
        selection_.setSelection(unified::objectIdsBelow(result_, index), services::SelectionSource::Tree);
    });
    if (node.model != 0 && node.ifc_id != 0) {
        menu.addItem("properties", "Show properties", [this, node] {
            session_.inspectEntity(node.model, node.ifc_id);
        });
    }
    menu.exec(tree->viewport()->mapToGlobal(pos), tree);
}

} // namespace bonsaiviewer::modules::unified_tree
