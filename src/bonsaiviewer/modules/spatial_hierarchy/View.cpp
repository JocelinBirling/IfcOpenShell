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

#include "Panel.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../../ifcviewer/SceneLoader.h"
#include "../../../ifcviewer/ViewportWindow.h"
#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"

#include "element.h"    // helpers: get_spatial_children, get_contained_elements, get_string_attribute
#include "placement.h"  // helpers: get_storey_elevation

#include <QCollator>
#include <QHash>

#include <algorithm>

namespace bonsaiviewer::modules::spatial_hierarchy {

namespace {

// Sort siblings by name with natural ordering (so "Level 2" precedes "Level 10").
void sortByName(QList<TreeNode>& nodes) {
    static const QCollator collator = [] {
        QCollator c;
        c.setNumericMode(true);
        c.setCaseSensitivity(Qt::CaseInsensitive);
        return c;
    }();
    std::sort(nodes.begin(), nodes.end(), [](const TreeNode& a, const TreeNode& b) {
        return collator.compare(a.name, b.name) < 0;
    });
}

TreeNode* findNodeRecursive(QList<TreeNode>& nodes, const NodePath& path, int depth) {
    for (auto& node : nodes) {
        if (node.name != path.at(depth)) continue;
        if (depth == path.size() - 1) return &node;
        return findNodeRecursive(node.children, path, depth + 1);
    }
    return nullptr;
}

ItemKind kindOf(const express::base& element) {
    const auto& declaration = element.declaration();
    if (declaration.is("IfcSite")) return ItemKind::Site;
    if (declaration.is("IfcBuilding")) return ItemKind::Building;
    if (declaration.is("IfcBuildingStorey")) return ItemKind::Storey;
    return ItemKind::Space;  // IfcSpace, IfcSpatialZone, …
}

QString displayName(const express::base& element) {
    // Name first; many files leave the spatial levels' Name empty and
    // only fill LongName — show that rather than the class name.
    if (auto name = get_string_attribute(element, "Name"); name && !name->empty()) {
        return QString::fromStdString(*name);
    }
    if (auto long_name = get_string_attribute(element, "LongName"); long_name && !long_name->empty()) {
        return QString::fromStdString(*long_name);
    }
    return QString::fromStdString(element.declaration().name());
}

TreeNode buildNode(const express::base& element) {
    TreeNode node;
    node.name = displayName(element);
    node.kind = kindOf(element);
    node.ifc_id = static_cast<int>(element.id());
    node.visible = true;
    // Secondary column: the storey elevation, else the LongName when filled.
    if (node.kind == ItemKind::Storey) {
        node.detail = QString::number(get_storey_elevation(element));
    } else if (auto long_name = get_string_attribute(element, "LongName");
               long_name && !long_name->empty()) {
        node.detail = QString::fromStdString(*long_name);
    }
    for (const auto& child : get_spatial_children(element)) {
        node.children.append(buildNode(child));
    }
    sortByName(node.children);
    return node;
}

// Every EXPRESS id in a spatial subtree: the element itself, its spatial
// children, and the elements each level contains (walls, doors, …).
void collectSubtreeIfcIds(const express::base& element, std::vector<int>& ids) {
    if (!element) return;
    ids.push_back(static_cast<int>(element.id()));
    for (const auto& contained : get_contained_elements(element)) {
        collectSubtreeIfcIds(contained, ids);
    }
    for (const auto& child : get_spatial_children(element)) {
        collectSubtreeIfcIds(child, ids);
    }
}

} // namespace

SpatialHierarchyPanelView::SpatialHierarchyPanelView(SpatialHierarchyPanel* widget,
                                                     bonsaiviewer::SessionState* session_state,
                                                     ViewportWindow* viewport,
                                                     QObject* parent)
    : QObject(parent), widget_(widget), session_state_(session_state), viewport_(viewport)
{
    connect(widget_, &SpatialHierarchyPanel::visibilityToggleRequested, this, [this](const NodePath& path) {
        if (auto* node = findNode(path)) {
            node->visible = !node->visible;
            applyVisibility(*node);
            reload();
            session_state_->setStatusMessage("Spatial", node->visible ? "Item shown" : "Item hidden");
        }
    });

    // The tree reflects the active model only. Rebuild when it changes, when its
    // geometry or its live IFC data source arrives (the .ifc for a .ifcview hit
    // loads asynchronously), and on project open/reset.
    connect(session_state_, &bonsaiviewer::SessionState::activeModelChanged, this, [this](const QString&) { rebuild(); });
    connect(session_state_, &bonsaiviewer::SessionState::modelDataSourceReady, this, [this](uint32_t) { rebuild(); });
    connect(session_state_, &bonsaiviewer::SessionState::modelGeometryReady, this, [this](uint32_t) { rebuild(); });
    connect(session_state_, &bonsaiviewer::SessionState::projectOpened, this, [this](const QString&) { rebuild(); });
    connect(session_state_, &bonsaiviewer::SessionState::projectReset, this, [this]() { rebuild(); });

    rebuild();
}

void SpatialHierarchyPanelView::rebuild() {
    nodes_.clear();

    auto* loader = session_state_->loader();
    const QString active_model_id = session_state_->activeModelId();
    if (loader != nullptr && !active_model_id.isEmpty()) {
        const uint32_t session_model_id = session_state_->sessionModelIdForModelId(active_model_id);
        ifcopenshell::file* file = session_model_id != 0 ? loader->ifcFile(session_model_id) : nullptr;
        if (file != nullptr) {  // null for a geometry-only model with no live IFC
            try {
                // IfcProject → IfcSite → … ; start the tree at the project's
                // spatial children (the project itself has no ItemKind).
                for (const auto& project : file->instances_by_type("IfcProject")) {
                    for (const auto& child : get_spatial_children(project)) {
                        nodes_.append(buildNode(child));
                    }
                }
            } catch (const std::exception&) {
                // Unsupported schema or malformed decomposition — show nothing.
            }
        }
    }

    sortByName(nodes_);
    reload();
}

void SpatialHierarchyPanelView::reload() {
    widget_->setNodes(nodes_);
}

TreeNode* SpatialHierarchyPanelView::findNode(const NodePath& path) {
    if (path.isEmpty()) return nullptr;
    return findNodeRecursive(nodes_, path, 0);
}

void SpatialHierarchyPanelView::applyVisibility(const TreeNode& node) {
    if (!viewport_ || node.ifc_id == 0) return;
    auto* loader = session_state_->loader();
    const QString active_model_id = session_state_->activeModelId();
    if (loader == nullptr || active_model_id.isEmpty()) return;
    const uint32_t session_model_id = session_state_->sessionModelIdForModelId(active_model_id);
    ifcopenshell::file* file = session_model_id != 0 ? loader->ifcFile(session_model_id) : nullptr;
    if (file == nullptr) return;

    // The subtree's EXPRESS ids (contained elements included), resolved to
    // the session's object ids through the element registry.
    std::vector<int> ifc_ids;
    collectSubtreeIfcIds(file->instance_by_id(node.ifc_id), ifc_ids);
    QHash<int, uint32_t> object_id_by_ifc_id;
    for (const auto& info : session_state_->elementRegistry()->basicElementInfoForModel(session_model_id)) {
        object_id_by_ifc_id.insert(info.ifc_id, info.object_id);
    }
    std::vector<uint32_t> object_ids;
    object_ids.reserve(ifc_ids.size());
    for (int id : ifc_ids) {
        const auto found = object_id_by_ifc_id.find(id);
        if (found != object_id_by_ifc_id.end()) object_ids.push_back(found.value());
    }
    if (!object_ids.empty()) {
        viewport_->setObjectsVisible(object_ids, node.visible);
    }
}

} // namespace bonsaiviewer::modules::spatial_hierarchy
