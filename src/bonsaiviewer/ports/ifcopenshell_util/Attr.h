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

// PORT — support code for the C++ ports of ifcopenshell.util.* used by
// IfcTester. The Python code reads entities by attribute name
// (getattr(element, "Decomposes", None)); these helpers do the same on the
// schema-agnostic express API, so every port stays a line-by-line copy of
// the Python, for every schema, without schema templates.

#ifndef BONSAIVIEWER_PORTS_IFCOPENSHELL_UTIL_ATTR_H
#define BONSAIVIEWER_PORTS_IFCOPENSHELL_UTIL_ATTR_H

#include "../../../ifcparse/express.h"
#include "../../../ifcparse/instance_data.h"

#include <optional>
#include <string>
#include <vector>

namespace ifcopenshell { class file; }

namespace bonsaiviewer::ports::ifcopenshell_util {

using Entity = express::base;

// element.is_a(ifc_class) (subtypes included), and element.is_a() .
bool isA(const Entity& element, const std::string& ifc_class);
std::string className(const Entity& element);

// The schema of the element's file: "IFC2X3", "IFC4", "IFC4X3_ADD2"...
std::string schemaName(const Entity& element);

// Attribute category of element.get_attribute_category(name): 1 forward,
// 2 inverse, 0 unknown.
int attributeCategory(const Entity& element, const std::string& name);

// getattr(element, name, None) for a forward attribute: nullopt when the
// entity has no such attribute (or it is unset).
std::optional<ifcopenshell::attribute_value> forward(const Entity& element, const std::string& name);

// A forward attribute holding one entity (None -> empty Entity).
Entity forwardEntity(const Entity& element, const std::string& name);
// A forward attribute holding entities (SET / LIST), empty when unset.
std::vector<Entity> forwardEntities(const Entity& element, const std::string& name);
// A forward attribute holding a string or an enumeration ("" when unset).
std::optional<std::string> forwardString(const Entity& element, const std::string& name);

// getattr(element, inverse_name, ()) — empty when the entity has no such inverse.
std::vector<Entity> inverse(const Entity& element, const std::string& name);

// The forward attribute names of the entity, in schema order (get_info()).
std::vector<std::string> forwardAttributeNames(const Entity& element);

// The wrapped value of a defined-type instance (IfcLabel('x').wrappedValue).
ifcopenshell::attribute_value wrappedValue(const Entity& defined_type_instance);

} // namespace bonsaiviewer::ports::ifcopenshell_util

#endif
