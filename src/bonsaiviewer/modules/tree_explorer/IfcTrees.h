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

#ifndef IFCINTERFACE_MODULES_TREE_EXPLORER_IFCTREES_H
#define IFCINTERFACE_MODULES_TREE_EXPLORER_IFCTREES_H

#include "../../services/TreeIndex.h"

#include <QHash>
#include <QString>

#include <cstdint>

namespace ifcopenshell {
class file;
}

namespace bonsaiviewer::modules::tree_explorer::ifc_trees {

// BonsaiViewer32 — the trees read from the IFC relations of one model. Each
// builder adds its nodes under `parent` (-1: at the root). No Qt widgets and
// no session: unit tested on IFC2X3 files. Schema-agnostic (every schema).
struct ModelInput {
    std::uint32_t model = 0;
    ifcopenshell::file* file = nullptr;
    // EXPRESS id -> viewport object id, for the elements with geometry.
    QHash<int, std::uint32_t> object_by_ifc;
};

// Site > Building > Storey > Space > contained elements (and their parts).
void buildSpatial(bonsaiviewer::services::tree::TreeIndex& index, const ModelInput& input, int parent);
// IFC class of the type > type > occurrences.
void buildTypes(bonsaiviewer::services::tree::TreeIndex& index, const ModelInput& input, int parent);

enum class GroupKind {
    Systems,  // IfcSystem and subtypes, except IfcZone
    Zones,    // IfcZone
    Groups,   // the other IfcGroup: IfcAsset, IfcInventory, IfcGroup...
};
// Group > sub-groups of the same kind > members.
void buildGroups(bonsaiviewer::services::tree::TreeIndex& index, const ModelInput& input, int parent,
                 GroupKind kind);

// Material or material set > materials of the set (layers, profiles,
// constituents) > elements. Usages show their set; occurrences without
// their own material are listed under the material of their type.
void buildMaterials(bonsaiviewer::services::tree::TreeIndex& index, const ModelInput& input, int parent);

} // namespace bonsaiviewer::modules::tree_explorer::ifc_trees

#endif
