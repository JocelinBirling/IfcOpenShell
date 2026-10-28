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

// PORT of src/ifctester/ifctester/facet.py — Restriction, cast_to_value,
// is_x — and of elementpath.regex.translate_pattern (the XSD pattern
// dialect of IDS, translated to the PCRE of QRegularExpression).

#ifndef BONSAIVIEWER_PORTS_IFCTESTER_RESTRICTION_H
#define BONSAIVIEWER_PORTS_IFCTESTER_RESTRICTION_H

#include "PyValue.h"

#include <string>
#include <utility>
#include <vector>

namespace bonsaiviewer::ports::ifctester {

// facet.py: cast_to_value(from_value, to_value) — the IDS text cast to the
// Python type of the IFC value (int -> float, "true"/"1" -> True...). None
// (an empty Value) where Python returns None.
Value castToValue(const std::string& from_value, const Value& to_value);

// facet.py: is_x(value, cast_value) — reals equal within 1e-6 relative.
bool isX(double value, double cast_value);

// elementpath translate_pattern(pattern, back_references=False,
// lazy_quantifiers=False, anchors=False): XSD regex -> PCRE, to be matched
// on the whole string.
std::string translateXsdPattern(const std::string& pattern);

// `text` fully matches the XSD `pattern` (re.fullmatch of the translation).
bool xsdFullMatch(const std::string& pattern, const std::string& text);

// facet.py: class Restriction. `options` keeps the XSD facets in document
// order, each with its value(s) as text: enumeration, pattern, length,
// minLength, maxLength, minInclusive, minExclusive, maxInclusive,
// maxExclusive, totalDigits, fractionDigits.
struct Restriction {
    std::string base = "string";  // without the "xs:" prefix
    std::vector<std::pair<std::string, std::vector<std::string>>> options;

    const std::vector<std::string>* option(const std::string& key) const;
    void add(const std::string& key, const std::string& value);

    // Restriction.__eq__(other): `other` passes every facet of the restriction.
    bool equals(const Value& other) const;
    // Restriction.__str__
    std::string toString() const;
};

// A facet parameter of IfcTester: None, a simpleValue (str) or a Restriction.
struct Param {
    enum class Kind { None, Simple, Restriction };
    Kind kind = Kind::None;
    std::string simple;
    Restriction restriction;

    static Param none() { return {}; }
    static Param text(const std::string& value);
    static Param of(const Restriction& restriction);

    bool isNone() const { return kind == Kind::None; }
    bool isSimple() const { return kind == Kind::Simple; }
    bool isRestriction() const { return kind == Kind::Restriction; }
    // Python truthiness: `if self.value:` (an empty str is false).
    bool truthy() const { return kind == Kind::Restriction || (kind == Kind::Simple && !simple.empty()); }
    // Python `other == self.param` (str equality, or Restriction.__eq__).
    bool matches(const Value& other) const;
    // str(param), for the messages.
    std::string toString() const;
};

} // namespace bonsaiviewer::ports::ifctester

#endif
