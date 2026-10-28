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

// PORT — support code for the C++ port of IfcTester (src/ifctester/ifctester).
// IfcTester compares Python values (None, bool, int, float, str, entity
// instances, tuples); this header gives the same operations on the
// property_value of the C++ pset helpers (src/helpers/pset.h):
//   ==, str(), float(), and the IfcOpenShell attribute -> Python conversion.
// Keep the behaviour aligned with Python, not with what would be "nicer".

#ifndef BONSAIVIEWER_PORTS_IFCTESTER_PYVALUE_H
#define BONSAIVIEWER_PORTS_IFCTESTER_PYVALUE_H

#include "pset.h"  // helpers: property_value

#include "../../../ifcparse/express.h"
#include "../../../ifcparse/instance_data.h"

#include <optional>
#include <string>

namespace bonsaiviewer::ports::ifctester {

using Value = property_value;

// Python `a == b`: numbers (bool, int, float) compare by value, strings by
// text, entity instances by identity, tuples element-wise; None == None.
bool pyEq(const Value& a, const Value& b);

// Python `str(value)`: True / False, repr of floats ("1.0", "1e-05"), None.
std::string pyStr(const Value& value);

// Python `repr(float)`.
std::string pyFloatRepr(double value);

// Python `float(value)`; nullopt where Python raises (ValueError / TypeError).
std::optional<double> pyFloat(const Value& value);

// Python `float(text)` for a string.
std::optional<double> pyFloatFromString(const std::string& text);

// Python `int(text)` for a string; nullopt on ValueError.
std::optional<long long> pyIntFromString(const std::string& text);

// Number of characters (code points) of a UTF-8 string: Python `len(str)`.
std::size_t pyLen(const std::string& utf8);

// The Python value IfcOpenShell gives for an attribute: defined types are
// unwrapped, select values stay entity instances (with is_a / wrappedValue),
// LOGICAL UNKNOWN is the string "UNKNOWN", aggregates are tuples.
Value fromAttribute(const ifcopenshell::attribute_value& value);

bool isNone(const Value& value);
bool isString(const Value& value);
bool isList(const Value& value);
bool isEntity(const Value& value);

} // namespace bonsaiviewer::ports::ifctester

#endif
