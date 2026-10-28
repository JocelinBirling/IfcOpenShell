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

// PORT of src/ifcopenshell-python/ifcopenshell/util/element.py — the
// functions IfcTester uses, line by line on the schema-agnostic API
// (Attr.h). The schema-templated helpers of src/helpers (element.h) cover
// part of them with simplifications; IfcTester needs the exact Python
// behaviour (get_parent, the indirect get_container...), hence this copy.

#ifndef BONSAIVIEWER_PORTS_IFCOPENSHELL_UTIL_ELEMENT_H
#define BONSAIVIEWER_PORTS_IFCOPENSHELL_UTIL_ELEMENT_H

#include "Attr.h"

#include <optional>
#include <string>
#include <vector>

namespace bonsaiviewer::ports::ifcopenshell_util {

Entity get_type(const Entity& element);
std::vector<Entity> get_types(const Entity& type);
// None when the element has no predefined type.
std::optional<std::string> get_predefined_type(const Entity& element);
bool is_userdefined_type(const Entity& element);

Entity get_aggregate(const Entity& element);
Entity get_nest(const Entity& element);
Entity get_container(const Entity& element, bool should_get_direct = false,
                     const std::string& ifc_class = std::string());
Entity get_filled_void(const Entity& element);
Entity get_voided_element(const Entity& element);
Entity get_adhered_element(const Entity& element);
Entity get_parent(const Entity& element, const std::string& ifc_class = std::string());

Entity get_material(const Entity& element, bool should_skip_usage = false, bool should_inherit = true);

} // namespace bonsaiviewer::ports::ifcopenshell_util

#endif
