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

#include "IfcTrees.h"

#include "../../ports/ifcopenshell_util/Attr.h"
#include "../../../ifcparse/file.h"

#include "element.h"  // helpers: get_spatial_children, get_contained_elements, get_parts, get_group_members...

#include <QSet>

#include <vector>

namespace bonsaiviewer::modules::tree_explorer::ifc_trees {

using bonsaiviewer::ports::ifcopenshell_util::className;
using bonsaiviewer::ports::ifcopenshell_util::Entity;
using bonsaiviewer::ports::ifcopenshell_util::forward;
using bonsaiviewer::ports::ifcopenshell_util::forwardEntities;
using bonsaiviewer::ports::ifcopenshell_util::forwardEntity;
using bonsaiviewer::ports::ifcopenshell_util::forwardString;
using bonsaiviewer::ports::ifcopenshell_util::inverse;
using bonsaiviewer::ports::ifcopenshell_util::isA;
using bonsaiviewer::services::tree::IndexNode;
using bonsaiviewer::services::tree::TreeIndex;

namespace {

const QString kFolderIcon = QStringLiteral(":/icons/folder.svg");
const QString kElementIcon = QStringLiteral(":/icons/cube.svg");
constexpr int kMaxDepth = 64;

QString attributeText(const Entity& entity, const char* name) {
    const auto value = forwardString(entity, name);
    return value ? QString::fromStdString(*value).trimmed() : QString();
}

// Name, else LongName, else the IFC class.
QString nameOf(const Entity& entity) {
    QString name = attributeText(entity, "Name");
    if (name.isEmpty()) name = attributeText(entity, "LongName");
    if (name.isEmpty()) name = QString::fromStdString(className(entity));
    return name;
}

QString tooltipOf(const Entity& entity) {
    return QStringLiteral("%1 #%2").arg(QString::fromStdString(className(entity))).arg(entity.id());
}

IndexNode entityNode(const Entity& entity, const ModelInput& input, IndexNode::Kind kind, const QString& icon) {
    IndexNode node;
    node.kind = kind;
    node.id = QStringLiteral("e%1").arg(entity.id());
    node.label = nameOf(entity);
    node.tooltip = tooltipOf(entity);
    node.icon = icon;
    node.model = input.model;
    node.ifc_id = static_cast<int>(entity.id());
    node.object_id = input.object_by_ifc.value(node.ifc_id, 0);
    return node;
}

IndexNode labelFolder(const QString& id, const QString& label, const QString& icon = kFolderIcon) {
    IndexNode node;
    node.kind = IndexNode::Kind::Folder;
    node.id = id;
    node.label = label;
    node.icon = icon;
    return node;
}

// An element and, below it, its parts (a curtain wall's plates...).
void addElement(TreeIndex& index, const ModelInput& input, const Entity& element, int parent, int depth = 0) {
    const int node = index.add(parent, entityNode(element, input, IndexNode::Kind::Element, kElementIcon));
    if (depth >= kMaxDepth) return;
    for (const Entity& part : get_parts(element)) addElement(index, input, part, node, depth + 1);
}

std::vector<Entity> instancesOf(ifcopenshell::file* file, const char* ifc_class) {
    try {
        return file->instances_by_type(ifc_class);
    } catch (const std::exception&) {
        return {};  // not in this schema
    }
}

// ---- Spatial ----------------------------------------------------------------

QString spatialIcon(const Entity& entity) {
    if (isA(entity, "IfcSite")) return QStringLiteral(":/icons/frame-alt.svg");
    if (isA(entity, "IfcBuilding")) return QStringLiteral(":/icons/city.svg");
    if (isA(entity, "IfcBuildingStorey")) return QStringLiteral(":/icons/planimetry.svg");
    return QStringLiteral(":/icons/square3d-from-center.svg");
}

void addSpatial(TreeIndex& index, const ModelInput& input, const Entity& spatial, int parent, int depth) {
    IndexNode node = entityNode(spatial, input, IndexNode::Kind::Folder, spatialIcon(spatial));
    // Sites and buildings open, down to the storeys.
    node.expanded = isA(spatial, "IfcSite") || isA(spatial, "IfcBuilding");
    if (isA(spatial, "IfcBuildingStorey")) {
        if (const auto elevation = forward(spatial, "Elevation");
            elevation && elevation->type() == ifcopenshell::Argument_DOUBLE) {
            node.tooltip += QStringLiteral(", elevation %1").arg(static_cast<double>(*elevation));
        }
    }
    const int folder = index.add(parent, node);
    if (depth >= kMaxDepth) return;
    for (const Entity& child : get_spatial_children(spatial)) addSpatial(index, input, child, folder, depth + 1);
    for (const Entity& element : get_contained_elements(spatial)) addElement(index, input, element, folder);
}

// ---- Groups -----------------------------------------------------------------

bool isKind(const Entity& entity, GroupKind kind) {
    switch (kind) {
    case GroupKind::Systems: return isA(entity, "IfcSystem") && !isA(entity, "IfcZone");
    case GroupKind::Zones: return isA(entity, "IfcZone");
    // IfcZone is an IfcSystem from IFC4, an IfcGroup in IFC2X3.
    case GroupKind::Groups: return isA(entity, "IfcGroup") && !isA(entity, "IfcSystem") && !isA(entity, "IfcZone");
    }
    return false;
}

// The sub-groups: members of the same kind, and (IFC4) aggregated parts.
std::vector<Entity> subGroupsAndMembers(const Entity& group) {
    std::vector<Entity> members = get_group_members(group);
    for (const Entity& part : get_parts(group)) members.push_back(part);
    return members;
}

bool hasParentGroup(const Entity& group, GroupKind kind) {
    for (const Entity& parent : get_groups(group)) {
        if (isKind(parent, kind)) return true;
    }
    const Entity aggregate = get_aggregate(group);
    return aggregate && isKind(aggregate, kind);
}

void addGroup(TreeIndex& index, const ModelInput& input, const Entity& group, GroupKind kind, int parent,
              QSet<int>& path) {
    const int folder = index.add(parent, entityNode(group, input, IndexNode::Kind::Folder, kFolderIcon));
    path.insert(static_cast<int>(group.id()));
    for (const Entity& member : subGroupsAndMembers(group)) {
        if (isKind(member, kind)) {
            if (!path.contains(static_cast<int>(member.id())) && path.size() < kMaxDepth) {
                addGroup(index, input, member, kind, folder, path);
            }
        } else {
            addElement(index, input, member, folder);
        }
    }
    path.remove(static_cast<int>(group.id()));
}

// ---- Materials --------------------------------------------------------------

QString materialName(const Entity& material) {
    if (!material) return QStringLiteral("No material");
    return nameOf(material);
}

QString thicknessText(const Entity& layer) {
    const auto value = forward(layer, "LayerThickness");
    if (!value || value->type() != ifcopenshell::Argument_DOUBLE) return {};
    return QStringLiteral(" (%1)").arg(static_cast<double>(*value));
}

IndexNode materialFolder(const Entity& entity, const QString& label) {
    IndexNode node = labelFolder(QStringLiteral("e%1").arg(entity.id()), label);
    node.tooltip = tooltipOf(entity);
    return node;
}

// The node of a material definition and the nodes the elements go under:
// the definition itself for a material, its layers / profiles / constituents
// for a set.
std::vector<int> addMaterialDefinition(TreeIndex& index, const ModelInput& input, const Entity& definition,
                                       int parent) {
    auto withModel = [&input](IndexNode node, const Entity& entity) {
        node.model = input.model;
        node.ifc_id = static_cast<int>(entity.id());
        return node;
    };
    std::vector<int> leaves;
    // A set: one folder, one child per item of the set.
    auto addSet = [&](const QString& label, const std::vector<Entity>& items, auto itemLabel) {
        const int set = index.add(parent, withModel(materialFolder(definition, label), definition));
        for (const Entity& item : items) {
            leaves.push_back(index.add(set, withModel(materialFolder(item, itemLabel(item)), item)));
        }
    };
    const auto namedOr = [](const Entity& entity, const QString& fallback) {
        const QString name = attributeText(entity, "Name");
        return name.isEmpty() ? fallback : name;
    };

    if (isA(definition, "IfcMaterialLayerSet")) {
        QString label = attributeText(definition, "LayerSetName");
        if (label.isEmpty()) label = QStringLiteral("Layer set #%1").arg(definition.id());
        addSet(label, forwardEntities(definition, "MaterialLayers"), [&](const Entity& layer) {
            return namedOr(layer, materialName(forwardEntity(layer, "Material"))) + thicknessText(layer);
        });
    } else if (isA(definition, "IfcMaterialProfileSet")) {
        addSet(namedOr(definition, QStringLiteral("Profile set #%1").arg(definition.id())),
               forwardEntities(definition, "MaterialProfiles"), [&](const Entity& profile) {
                   return namedOr(profile, materialName(forwardEntity(profile, "Material")));
               });
    } else if (isA(definition, "IfcMaterialConstituentSet")) {
        addSet(namedOr(definition, QStringLiteral("Constituent set #%1").arg(definition.id())),
               forwardEntities(definition, "MaterialConstituents"), [&](const Entity& constituent) {
                   return namedOr(constituent, materialName(forwardEntity(constituent, "Material")));
               });
    } else if (isA(definition, "IfcMaterialList")) {
        addSet(QStringLiteral("Material list #%1").arg(definition.id()), forwardEntities(definition, "Materials"),
               [&](const Entity& material) { return materialName(material); });
    } else if (isA(definition, "IfcMaterialLayer")) {
        leaves.push_back(index.add(parent, withModel(materialFolder(definition,
            namedOr(definition, materialName(forwardEntity(definition, "Material"))) + thicknessText(definition)),
            definition)));
    } else if (isA(definition, "IfcMaterialProfile") || isA(definition, "IfcMaterialConstituent")) {
        leaves.push_back(index.add(parent, withModel(materialFolder(definition,
            namedOr(definition, materialName(forwardEntity(definition, "Material")))), definition)));
    } else {
        // IfcMaterial, and any definition not known here.
        leaves.push_back(index.add(parent, withModel(materialFolder(definition, materialName(definition)), definition)));
    }
    return leaves;
}

bool hasOwnMaterial(const Entity& element) {
    for (const Entity& association : inverse(element, "HasAssociations")) {
        if (isA(association, "IfcRelAssociatesMaterial")) return true;
    }
    return false;
}

} // namespace

void buildSpatial(TreeIndex& index, const ModelInput& input, int parent) {
    for (const Entity& project : instancesOf(input.file, "IfcProject")) {
        for (const Entity& child : get_spatial_children(project)) addSpatial(index, input, child, parent, 0);
    }
}

void buildTypes(TreeIndex& index, const ModelInput& input, int parent) {
    QHash<QString, int> class_folders;
    for (const Entity& type : instancesOf(input.file, "IfcTypeObject")) {
        const QString ifc_class = QString::fromStdString(className(type));
        int folder = class_folders.value(ifc_class, -1);
        if (folder < 0) {
            folder = index.add(parent, labelFolder(ifc_class, ifc_class));
            class_folders.insert(ifc_class, folder);
        }
        const int type_node = index.add(folder, entityNode(type, input, IndexNode::Kind::Folder, kFolderIcon));
        for (const Entity& occurrence : get_type_occurrences(type)) addElement(index, input, occurrence, type_node);
    }
}

void buildGroups(TreeIndex& index, const ModelInput& input, int parent, GroupKind kind) {
    for (const Entity& group : instancesOf(input.file, "IfcGroup")) {
        if (!isKind(group, kind) || hasParentGroup(group, kind)) continue;
        QSet<int> path;
        addGroup(index, input, group, kind, parent, path);
    }
}

void buildMaterials(TreeIndex& index, const ModelInput& input, int parent) {
    // One node per definition, whatever the number of relations using it;
    // a usage shows its set.
    QHash<int, std::vector<int>> leaves_of;
    for (const Entity& relation : instancesOf(input.file, "IfcRelAssociatesMaterial")) {
        Entity definition = forwardEntity(relation, "RelatingMaterial");
        if (!definition) continue;
        if (isA(definition, "IfcMaterialLayerSetUsage")) definition = forwardEntity(definition, "ForLayerSet");
        else if (isA(definition, "IfcMaterialProfileSetUsage")) definition = forwardEntity(definition, "ForProfileSet");
        if (!definition) continue;
        const int definition_id = static_cast<int>(definition.id());
        if (!leaves_of.contains(definition_id)) {
            leaves_of.insert(definition_id, addMaterialDefinition(index, input, definition, parent));
        }
        const std::vector<int> leaves = leaves_of.value(definition_id);
        for (const Entity& object : forwardEntities(relation, "RelatedObjects")) {
            std::vector<Entity> elements;
            if (isA(object, "IfcTypeObject")) {
                // The material of a type goes to its occurrences without their own.
                for (const Entity& occurrence : get_type_occurrences(object)) {
                    if (!hasOwnMaterial(occurrence)) elements.push_back(occurrence);
                }
            } else {
                elements.push_back(object);
            }
            for (int leaf : leaves) {
                for (const Entity& element : elements) {
                    index.add(leaf, entityNode(element, input, IndexNode::Kind::Element, kElementIcon));
                }
            }
        }
    }
}

} // namespace bonsaiviewer::modules::tree_explorer::ifc_trees
