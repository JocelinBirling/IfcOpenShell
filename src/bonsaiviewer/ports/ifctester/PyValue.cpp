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

// PORT — see PyValue.h.

#include "PyValue.h"

#include "../../../ifcparse/schema.h"

#include <boost/dynamic_bitset.hpp>
#include <boost/logic/tribool.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace bonsaiviewer::ports::ifctester {

namespace {

bool isNumber(const Value& value) {
    return value.get_if<bool>() || value.get_if<std::int64_t>() || value.get_if<double>();
}

double asNumber(const Value& value) {
    if (const auto* b = value.get_if<bool>()) return *b ? 1.0 : 0.0;
    if (const auto* i = value.get_if<std::int64_t>()) return static_cast<double>(*i);
    return *value.get_if<double>();
}

std::string trim(const std::string& text) {
    std::size_t begin = 0, end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(begin, end - begin);
}

// Python allows "_" between digits in numeric literals given to float()/int().
bool stripUnderscores(const std::string& text, std::string& out) {
    out.clear();
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '_') {
            out += text[i];
            continue;
        }
        const bool digit_before = i > 0 && std::isdigit(static_cast<unsigned char>(text[i - 1]));
        const bool digit_after = i + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[i + 1]));
        if (!digit_before || !digit_after) return false;
    }
    return true;
}

} // namespace

bool isNone(const Value& value) { return value.is_null(); }
bool isString(const Value& value) { return value.get_if<std::string>() != nullptr; }
bool isList(const Value& value) { return value.get_if<property_list>() != nullptr; }
bool isEntity(const Value& value) { return value.get_if<express::base>() != nullptr; }

bool pyEq(const Value& a, const Value& b) {
    if (a.is_null() || b.is_null()) return a.is_null() && b.is_null();
    if (isNumber(a) && isNumber(b)) return asNumber(a) == asNumber(b);
    if (const auto* sa = a.get_if<std::string>()) {
        const auto* sb = b.get_if<std::string>();
        return sb && *sa == *sb;
    }
    if (const auto* ea = a.get_if<express::base>()) {
        const auto* eb = b.get_if<express::base>();
        return eb && *ea == *eb;
    }
    if (const auto* la = a.get_if<property_list>()) {
        const auto* lb = b.get_if<property_list>();
        if (!lb || la->size() != lb->size()) return false;
        for (std::size_t i = 0; i < la->size(); ++i) {
            if (!pyEq((*la)[i], (*lb)[i])) return false;
        }
        return true;
    }
    if (const auto* ma = a.get_if<property_map>()) {
        const auto* mb = b.get_if<property_map>();
        if (!mb || ma->size() != mb->size()) return false;
        for (const auto& [key, value] : *ma) {
            const auto it = mb->find(key);
            if (it == mb->end() || !pyEq(value, it->second)) return false;
        }
        return true;
    }
    return false;
}

// repr(float): the shortest round-trip digits, fixed notation for
// 1e-4 <= |x| < 1e16, scientific otherwise ("1e-05", "1e+16").
std::string pyFloatRepr(double value) {
    if (std::isnan(value)) return "nan";
    if (std::isinf(value)) return value > 0 ? "inf" : "-inf";
    if (value == 0.0) return std::signbit(value) ? "-0.0" : "0.0";
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::scientific);
    std::string scientific(buffer, result.ptr);
    // scientific: [-]d[.ddd]e[+-]XX
    const std::size_t e = scientific.find('e');
    std::string mantissa = scientific.substr(0, e);
    const int exponent = std::stoi(scientific.substr(e + 1));
    const bool negative = mantissa[0] == '-';
    if (negative) mantissa.erase(0, 1);
    std::string digits;
    for (char c : mantissa) {
        if (c != '.') digits += c;
    }
    std::string out;
    if (exponent >= -4 && exponent < 16) {
        if (exponent >= 0) {
            if (static_cast<int>(digits.size()) <= exponent + 1) {
                out = digits + std::string(exponent + 1 - digits.size(), '0') + ".0";
            } else {
                out = digits.substr(0, exponent + 1) + "." + digits.substr(exponent + 1);
            }
        } else {
            out = "0." + std::string(-exponent - 1, '0') + digits;
        }
    } else {
        out = digits.substr(0, 1);
        if (digits.size() > 1) out += "." + digits.substr(1);
        char exp[16];
        std::snprintf(exp, sizeof(exp), "e%c%02d", exponent < 0 ? '-' : '+', std::abs(exponent));
        out += exp;
    }
    return negative ? "-" + out : out;
}

std::string pyStr(const Value& value) {
    if (value.is_null()) return "None";
    if (const auto* b = value.get_if<bool>()) return *b ? "True" : "False";
    if (const auto* i = value.get_if<std::int64_t>()) return std::to_string(*i);
    if (const auto* d = value.get_if<double>()) return pyFloatRepr(*d);
    if (const auto* s = value.get_if<std::string>()) return *s;
    if (const auto* e = value.get_if<express::base>()) {
        return *e ? "#" + std::to_string(e->id()) + "=" + e->declaration().name() + "(...)" : "None";
    }
    if (const auto* l = value.get_if<property_list>()) {
        std::string out = "(";
        for (std::size_t i = 0; i < l->size(); ++i) {
            if (i) out += ", ";
            const Value& item = (*l)[i];
            out += isString(item) ? "'" + pyStr(item) + "'" : pyStr(item);
        }
        return out + (l->size() == 1 ? ",)" : ")");
    }
    return "{...}";
}

std::optional<double> pyFloatFromString(const std::string& raw) {
    std::string text;
    if (!stripUnderscores(trim(raw), text) || text.empty()) return std::nullopt;
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    const std::string unsigned_part = (lower[0] == '+' || lower[0] == '-') ? lower.substr(1) : lower;
    const double sign = lower[0] == '-' ? -1.0 : 1.0;
    if (unsigned_part == "inf" || unsigned_part == "infinity") return sign * INFINITY;
    if (unsigned_part == "nan") return NAN;
    // Only digits, one dot, one exponent: what Python accepts (no hex, no "1.").
    char* end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (end != text.c_str() + text.size()) return std::nullopt;
    for (char c : lower) {
        if (c == 'x' || c == 'p' || c == 'n' || c == 'i') return std::nullopt;  // hex / words: not Python floats
    }
    return parsed;
}

std::optional<long long> pyIntFromString(const std::string& raw) {
    std::string text;
    if (!stripUnderscores(trim(raw), text) || text.empty()) return std::nullopt;
    std::size_t start = (text[0] == '+' || text[0] == '-') ? 1 : 0;
    if (start == text.size()) return std::nullopt;
    for (std::size_t i = start; i < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) return std::nullopt;
    }
    try {
        return std::stoll(text);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> pyFloat(const Value& value) {
    if (isNumber(value)) return asNumber(value);
    if (const auto* s = value.get_if<std::string>()) return pyFloatFromString(*s);
    return std::nullopt;
}

std::size_t pyLen(const std::string& utf8) {
    std::size_t count = 0;
    for (unsigned char c : utf8) {
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}

Value fromAttribute(const ifcopenshell::attribute_value& value) {
    if (value.isNull()) return {};
    switch (value.type()) {
    case ifcopenshell::Argument_INT:
        return static_cast<std::int64_t>(value);
    case ifcopenshell::Argument_BOOL:
        return static_cast<bool>(value);
    case ifcopenshell::Argument_LOGICAL: {
        const boost::logic::tribool logical = value;
        if (boost::logic::indeterminate(logical)) return std::string("UNKNOWN");
        return static_cast<bool>(logical);
    }
    case ifcopenshell::Argument_DOUBLE:
        return static_cast<double>(value);
    case ifcopenshell::Argument_STRING:
        return static_cast<std::string>(value);
    case ifcopenshell::Argument_ENUMERATION: {
        const ifcopenshell::enumeration_reference enumeration = value;
        return std::string(enumeration.value() ? enumeration.value() : "");
    }
    case ifcopenshell::Argument_BINARY: {
        const boost::dynamic_bitset<> bits = value;
        std::string text;
        boost::to_string(bits, text);
        return text;
    }
    case ifcopenshell::Argument_ENTITY_INSTANCE:
        return static_cast<express::base>(value);
    case ifcopenshell::Argument_EMPTY_AGGREGATE:
        return property_list{};
    case ifcopenshell::Argument_AGGREGATE_OF_INT: {
        property_list list;
        for (auto v : static_cast<std::vector<int64_t>>(value)) list.emplace_back(static_cast<std::int64_t>(v));
        return list;
    }
    case ifcopenshell::Argument_AGGREGATE_OF_DOUBLE: {
        property_list list;
        for (auto v : static_cast<std::vector<double>>(value)) list.emplace_back(v);
        return list;
    }
    case ifcopenshell::Argument_AGGREGATE_OF_STRING: {
        property_list list;
        for (auto& v : static_cast<std::vector<std::string>>(value)) list.emplace_back(v);
        return list;
    }
    case ifcopenshell::Argument_AGGREGATE_OF_ENTITY_INSTANCE: {
        property_list list;
        for (auto& v : static_cast<std::vector<express::base>>(value)) list.emplace_back(v);
        return list;
    }
    default:
        // Aggregates of aggregates: a non-empty tuple whose content IfcTester
        // never compares.
        return property_list{Value()};
    }
}

} // namespace bonsaiviewer::ports::ifctester
