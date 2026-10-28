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

// PORT of src/ifcopenshell-python/ifcopenshell/util/unit.py — the part
// IfcTester needs: si_type_names, get_measure_unit_type, get_project_unit,
// get_property_unit, get_property_table_unit. The conversion itself is the
// existing port of src/helpers (unit_convert.h: convert).

#ifndef BONSAIVIEWER_PORTS_IFCOPENSHELL_UTIL_UNIT_H
#define BONSAIVIEWER_PORTS_IFCOPENSHELL_UTIL_UNIT_H

#include "Attr.h"

#include <map>
#include <optional>
#include <string>

namespace bonsaiviewer::ports::ifcopenshell_util {

// si_type_names: unit type -> SI unit name ("LENGTHUNIT" -> "METRE").
const std::map<std::string, std::string>& si_type_names();

// def get_measure_unit_type(measure_class)
std::string get_measure_unit_type(const std::string& measure_class);

// def get_project_unit(ifc_file, unit_type) — without the derived unit
// dimensional match (identify_unit_dimensions), which IFC property units
// never need in practice.
Entity get_project_unit(const Entity& any_entity_of_the_file, const std::string& unit_type);

// def get_property_unit(prop, ifc_file)
Entity get_property_unit(const Entity& prop);

// def get_property_table_unit(prop, ifc_file): DefiningUnit, DefinedUnit.
std::pair<Entity, Entity> get_property_table_unit(const Entity& prop);

// unit.Name / unit.Prefix / unit.UnitType of a named unit (nullopt when absent).
std::optional<std::string> unitName(const Entity& unit);
std::optional<std::string> unitPrefix(const Entity& unit);
std::optional<std::string> unitType(const Entity& unit);

// def convert(value, from_prefix, from_unit, to_prefix, to_unit)
double convert(double value, const std::optional<std::string>& from_prefix, const std::string& from_unit,
               const std::optional<std::string>& to_prefix, const std::string& to_unit);

} // namespace bonsaiviewer::ports::ifcopenshell_util

#endif
