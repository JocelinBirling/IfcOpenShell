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

// PORT of src/ifcdiff/ifcdiff.py — class IfcDiff (BonsaiViewer34, TI34).
//
// Compares an old (Before) and a new (After) IFC file by GlobalId: the added,
// deleted and changed elements, with the relationships of the Python tool
// (attributes, geometry, type, property, container, aggregate,
// classification). The exported JSON keeps the Python schema
// ({"added", "deleted", "changed"}) and adds details next to the booleans.
//
// Differences with the Python tool, on purpose:
// - geometry is compared on the IFC data (placement, openings, projections,
//   Body representation items, attribute by attribute) instead of on the
//   triangulation of the kernel: no tessellation, and the detail of what
//   changed is kept;
// - container / aggregate are compared by GlobalId (Python compares entity
//   instances of two files, which never match);
// - with is_shallow off, every relationship is checked (Python stops at the
//   first different relationship either way).

#ifndef BONSAIVIEWER_SERVICES_IFCDIFF_IFCDIFF_H
#define BONSAIVIEWER_SERVICES_IFCDIFF_IFCDIFF_H

#include "../../ports/ifctester/Ids.h"

#include <QJsonObject>

#include <array>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace ifcopenshell { class file; }

namespace bonsaiviewer::services::ifcdiff {

// The relationships of RELATIONSHIP_TYPE, by their Python names.
inline const std::vector<std::string>& relationshipNames() {
    static const std::vector<std::string> names{"attributes", "geometry", "type", "property",
                                                "container", "aggregate", "classification"};
    return names;
}

// How a property of a common element changed (BonsaiViewer34 selection buttons).
enum class PropertyChangeKind {
    PropertyAdded,   // the property is in After only
    PropertyRemoved, // the property is in Before only
    ValueAdded,      // in both, no value in Before
    ValueRemoved,    // in both, no value in After
    ValueChanged,    // in both, two different values
};

struct PropertyChange {
    std::string pset;
    std::string property;
    std::string old_value; // as text, empty when there is none
    std::string new_value;
    PropertyChangeKind kind = PropertyChangeKind::ValueChanged;
    // ValueChanged between two numbers: +1 increased, -1 decreased; else 0.
    int direction = 0;
};

struct AttributeChange {
    std::string name;
    std::string old_value;
    std::string new_value;
};

// One attribute that differs inside a representation item, by its path from
// the item ("SweptArea.XDim", "Position.Location.Coordinates").
struct ItemAttributeChange {
    std::string path;
    std::string old_value;
    std::string new_value;
};

struct ItemChange {
    int index = 0;
    std::string type_old;
    std::string type_new;
    std::vector<ItemAttributeChange> attributes;
};

struct GeometryDetail {
    bool placement_changed = false; // moved or rotated
    bool moved = false;             // the translation differs (beyond the precision)
    std::array<double, 3> translation_old{};
    std::array<double, 3> translation_new{};
    bool rotation_changed = false;
    std::vector<std::string> openings_added;
    std::vector<std::string> openings_removed;
    std::vector<std::string> projections_added;
    std::vector<std::string> projections_removed;
    bool representation_changed = false;
    int representation_id_old = 0; // 0: no Body representation
    int representation_id_new = 0;
    // The Body representations have a different number of items.
    int items_count_old = 0;
    int items_count_new = 0;
    std::vector<ItemChange> items_changed;
};

struct ElementChange {
    std::string global_id;
    int old_id = 0; // EXPRESS ids in Before / After
    int new_id = 0;
    std::string ifc_class; // of the After element
    std::string name;

    bool attributes_changed = false;
    bool geometry_changed = false;
    bool type_changed = false;
    bool properties_changed = false;
    bool container_changed = false;
    bool aggregate_changed = false;
    bool classification_changed = false;

    std::vector<AttributeChange> attributes;
    std::vector<PropertyChange> properties;
    GeometryDetail geometry;

    // "attributes", "geometry", "property"... : the change kinds, in the
    // order of relationshipNames().
    std::vector<std::string> kinds() const;
};

// An element in one model only (added or deleted).
struct ElementRef {
    std::string global_id;
    int id = 0;
    std::string ifc_class;
    std::string name;
};

struct Options {
    std::set<std::string> relationships{"attributes", "geometry", "property"};
    bool is_shallow = false;
    // IDS shortlist: only the properties named in the requirements are
    // compared, and only on the elements the specifications apply to.
    std::optional<bonsaiviewer::ports::ifctester::Ids> ids;
    // Called with 0..100 during the run.
    std::function<void(int)> progress;
};

struct Result {
    std::vector<ElementRef> added;   // in After
    std::vector<ElementRef> deleted; // in Before
    std::map<std::string, ElementChange> changed; // by GlobalId
    double precision = 1e-4;
    int old_count = 0;
    int new_count = 0;

    bool empty() const { return added.empty() && deleted.empty() && changed.empty(); }
    // {"added": [...], "deleted": [...], "changed": {...}}, the IfcDiff CLI
    // format plus the details of each change.
    QJsonObject toJson() const;
};

// IfcDiff(old, new, relationships, is_shallow).diff()
Result diff(ifcopenshell::file& old_file, ifcopenshell::file& new_file, const Options& options);

std::string changeKindName(PropertyChangeKind kind);

} // namespace bonsaiviewer::services::ifcdiff

#endif // BONSAIVIEWER_SERVICES_IFCDIFF_IFCDIFF_H
