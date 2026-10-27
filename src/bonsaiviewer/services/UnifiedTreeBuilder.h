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

#ifndef IFCINTERFACE_SERVICES_UNIFIEDTREEBUILDER_H
#define IFCINTERFACE_SERVICES_UNIFIEDTREEBUILDER_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// BonsaiViewer13 — the Unified Tree computation. Pure: no Qt, no IFC, no
// viewport. The view collects the storeys, rooms and elements of the open
// models (names, world elevations, world AABBs) and this builder deduces the
// interpreted tree Site > Building > Level > Room > Elements.

namespace bonsaiviewer::services::unified {

struct Box {
    float mn[3] = {0, 0, 0};
    float mx[3] = {0, 0, 0};
};

// One IfcBuildingStorey of a model.
struct StoreyIn {
    std::uint32_t model = 0;
    std::string model_name;  // for the diagnostics and tooltips
    int ifc_id = 0;
    std::string name;
    // IfcBuildingStorey.Elevation in world metres; empty when missing.
    std::optional<double> elevation;
    // The site and building the storey belongs to (0 / empty when none).
    int site_ifc = 0;
    std::string site_name;
    int building_ifc = 0;
    std::string building_name;
};

// One IfcSpace of the rooms reference model.
struct RoomIn {
    int ifc_id = 0;
    std::uint32_t object_id = 0;  // 0 when the space has no geometry
    std::string name;
    Box box;
    // The room's triangles projected in plan (x0 y0 x1 y1 x2 y2 per triangle);
    // empty when its mesh is not available (the box is used instead).
    std::vector<float> footprint;
    int storey_ifc = 0;  // the storey of the space in its model, 0 when none
};

// One element with geometry, of any model.
struct ElementIn {
    std::uint32_t object_id = 0;
    std::uint32_t model = 0;
    int ifc_id = 0;
    std::string label;
    bool is_slab = false;
    bool is_wall = false;  // IfcWall*, IfcCurtainWall, IfcColumn: attached by contact
    Box box;
    // Walls and slabs only: their triangles projected in plan (as RoomIn).
    std::vector<float> footprint;
    int storey_ifc = 0;  // the storey containing it in its model, 0 when none
};

struct Settings {
    std::uint32_t levels_model = 0;
    std::uint32_t rooms_model = 0;
    double tolerance = 0.10;  // metres
    bool merge_sites = false;
    bool merge_buildings = false;
    bool merge_same_name = false;
    // An element is listed under the rooms of one level only: the level of its
    // IFC storey when one of its rooms is there, else the level where its box
    // overlaps the rooms the most (in height).
    bool one_level_per_element = true;
};

struct Input {
    Settings settings;
    std::vector<StoreyIn> storeys;  // every model
    std::vector<RoomIn> rooms;      // rooms reference model only
    std::vector<ElementIn> elements;
};

// Why a node sits where it is. None: read from the IFC as is.
enum class Reason {
    None,
    LevelByName,         // storey matched to the reference level of the same name
    LevelByNameMerged,   // same name, merged without the elevation check
    LevelByElevation,    // within the tolerance of the reference level
    LevelNearest,        // nearest reference level, outside the tolerance
    LevelCreated,        // new unified level
    RoomOverlap,         // element box overlaps the room box
    SlabBelowRoom,       // slab top at the room floor, boxes overlap in plan
    SlabTopFace,         // slab attached to the first level above its top face
    NoRoomFromStorey,    // no room: level of its IFC storey
};

struct Node {
    enum class Kind { Site, Building, Level, Room, NoRoom, NoLevel, Element, Diagnostics, Message };
    Kind kind = Kind::Element;
    std::string label;
    std::uint32_t model = 0;      // with ifc_id: the IFC entity behind the node
    int ifc_id = 0;
    std::uint32_t object_id = 0;  // Element, and Room with geometry
    Reason reason = Reason::None;
    std::string reason_text;      // tooltip: why the node is here
    int multiplicity = 1;         // Element: number of rooms it is listed in
    int count = 0;                // distinct elements below (each counted once)
    std::vector<int> children;    // indices in Result::nodes
};

struct Result {
    bool ok = false;
    std::string error;            // when !ok: why the run stopped
    std::vector<Node> nodes;
    std::vector<int> roots;
    std::vector<std::string> diagnostics;
};

Result build(const Input& input);

// Every object id below a node (the node itself included), each once.
std::vector<std::uint32_t> objectIdsBelow(const Result& result, int node);

} // namespace bonsaiviewer::services::unified

#endif
