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

// PORT — see Restriction.h.

#include "Restriction.h"

#include <QHash>
#include <QMutex>
#include <QRegularExpression>
#include <QString>

#include <cmath>

namespace bonsaiviewer::ports::ifctester {

// def cast_to_value(from_value, to_value)
Value castToValue(const std::string& from_value, const Value& to_value) {
    if (to_value.get_if<std::int64_t>()) {
        // Casting str -> float means that notation like '1e3' is preserved.
        // We do not cast to int because 42.0 == 42 and 42.3 != 42.
        if (const auto number = pyFloatFromString(from_value)) return *number;
        return {};
    }
    if (to_value.get_if<bool>()) {
        if (from_value == "true" || from_value == "1") return true;
        if (from_value == "false" || from_value == "0") return false;
        return {};
    }
    if (to_value.get_if<double>()) {
        if (const auto number = pyFloatFromString(from_value)) return *number;
        return {};
    }
    if (to_value.get_if<std::string>()) return from_value;
    if (to_value.get_if<property_list>()) {
        // builtins.list(str): the characters.
        property_list characters;
        const QString text = QString::fromStdString(from_value);
        for (const QChar c : text) characters.emplace_back(QString(c).toStdString());
        return characters;
    }
    // NoneType / entity_instance: no such builtin, IfcTester gets nothing.
    return {};
}

// def is_x(value, cast_value)
bool isX(double value, double cast_value) {
    if (cast_value >= 0) {
        if (value < cast_value * (1.0 - 1e-6) || value > cast_value * (1.0 + 1e-6)) return false;
    } else if (value > cast_value * (1.0 - 1e-6) || value < cast_value * (1.0 + 1e-6)) {
        return false;
    }
    return true;
}

namespace {

// XSD multi-character escapes, as PCRE class content (without brackets).
// \i: initial name characters, \c: name characters (XML 1.0 NameStartChar /
// NameChar, approximated with the Unicode letter and digit categories).
const char* kNameStart = "\\p{L}_:";
const char* kNameChar = "\\p{L}\\p{Nd}\\p{Mn}\\p{Mc}._:\\-\\x{B7}";

// Translate the escape at text[i] ('\\' at i). Writes either a class
// fragment (inside brackets) or a full atom (outside).
std::size_t translateEscape(const std::string& text, std::size_t i, bool in_class, std::string& out) {
    if (i + 1 >= text.size()) {
        out += "\\\\";
        return i + 1;
    }
    const char c = text[i + 1];
    auto cls = [&](const std::string& content, bool negated) {
        if (in_class) {
            // Negated classes inside a class: XSD allows them; PCRE needs a
            // nested negation, approximated by the positive content.
            out += negated ? std::string() : content;
        } else {
            out += std::string(negated ? "[^" : "[") + content + "]";
        }
    };
    switch (c) {
    case 'i': cls(kNameStart, false); return i + 2;
    case 'I': cls(kNameStart, true); return i + 2;
    case 'c': cls(kNameChar, false); return i + 2;
    case 'C': cls(kNameChar, true); return i + 2;
    // \d \D \w \W \s \S: elementpath keeps them as Python re escapes
    // (Unicode aware); PCRE with Unicode properties has the same classes.
    case 'd':
    case 'D':
    case 'w':
    case 'W':
    case 's':
    case 'S':
        out += '\\';
        out += c;
        return i + 2;
    case 'p':
    case 'P': {
        // \p{Lu}, \p{IsBasicLatin}: categories pass through; block names
        // (Is...) map to PCRE scripts where they exist.
        const std::size_t close = text.find('}', i);
        if (close == std::string::npos) {
            out += text.substr(i);
            return text.size();
        }
        std::string name = text.substr(i + 3, close - i - 3);
        if (name.rfind("Is", 0) == 0) name = name.substr(2);
        out += std::string("\\") + c + "{" + name + "}";
        return close + 1;
    }
    case 'n': out += "\\n"; return i + 2;
    case 'r': out += "\\r"; return i + 2;
    case 't': out += "\\t"; return i + 2;
    default:
        // Single-character escapes (\\ \. \- \^ \[ ...): literal.
        out += '\\';
        out += c;
        return i + 2;
    }
}

// Translate a character class starting at text[i] == '['. Supports XSD class
// subtraction "[base-[excluded]]" as a negative lookahead.
std::size_t translateClass(const std::string& text, std::size_t i, std::string& out) {
    std::string content;
    std::string subtraction;
    bool negated = false;
    std::size_t j = i + 1;
    if (j < text.size() && text[j] == '^') {
        negated = true;
        ++j;
    }
    while (j < text.size() && text[j] != ']') {
        if (text[j] == '-' && j + 1 < text.size() && text[j + 1] == '[') {
            // Subtraction: the rest up to the matching ']' is the excluded class.
            std::string excluded;
            j = translateClass(text, j + 1, excluded);
            subtraction = excluded;
            continue;
        }
        if (text[j] == '\\') {
            j = translateEscape(text, j, true, content);
            continue;
        }
        if (text[j] == '[' || (text[j] == '&' ) || (text[j] == '~')) {
            content += '\\';
        }
        content += text[j];
        ++j;
    }
    const std::string cls = std::string(negated ? "[^" : "[") + content + "]";
    if (subtraction.empty()) {
        out += cls;
    } else {
        out += "(?:(?!" + subtraction + ")" + cls + ")";
    }
    return j + 1;
}

} // namespace

std::string translateXsdPattern(const std::string& pattern) {
    std::string out;
    std::size_t i = 0;
    while (i < pattern.size()) {
        const char c = pattern[i];
        if (c == '\\') {
            i = translateEscape(pattern, i, false, out);
        } else if (c == '[') {
            i = translateClass(pattern, i, out);
        } else if (c == '.') {
            // XSD: any character but \n and \r.
            out += "[^\\n\\r]";
            ++i;
        } else if (c == '^' || c == '$') {
            // No anchors in XSD: literal characters.
            out += '\\';
            out += c;
            ++i;
        } else if (c == '(') {
            // Groups never capture for a full match; keep them plain.
            out += "(?:";
            ++i;
        } else {
            out += c;
            ++i;
        }
    }
    return out;
}

bool xsdFullMatch(const std::string& pattern, const std::string& text) {
    // translate_pattern is cached in IfcTester; so is the compiled expression here.
    static QMutex mutex;
    static QHash<QString, QRegularExpression> cache;
    const QString key = QString::fromStdString(pattern);
    QRegularExpression expression;
    {
        QMutexLocker lock(&mutex);
        auto it = cache.find(key);
        if (it == cache.end()) {
            const QString translated = QString::fromStdString("\\A(?:" + translateXsdPattern(pattern) + ")\\z");
            it = cache.insert(key, QRegularExpression(translated, QRegularExpression::UseUnicodePropertiesOption));
        }
        expression = it.value();
    }
    if (!expression.isValid()) return false;
    return expression.match(QString::fromStdString(text)).hasMatch();
}

const std::vector<std::string>* Restriction::option(const std::string& key) const {
    for (const auto& [name, values] : options) {
        if (name == key) return &values;
    }
    return nullptr;
}

void Restriction::add(const std::string& key, const std::string& value) {
    for (auto& [name, values] : options) {
        if (name == key) {
            values.push_back(value);
            return;
        }
    }
    options.push_back({key, {value}});
}

// def __eq__(self, other)
bool Restriction::equals(const Value& other) const {
    if (other.is_null()) return false;
    for (const auto& [constraint, values] : options) {
        if (values.empty()) continue;
        const std::string& value = values.front();
        if (constraint == "enumeration") {
            bool found = false;
            for (const std::string& option : values) {
                if (pyEq(other, castToValue(option, other))) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        } else if (constraint == "pattern") {
            const auto* text = other.get_if<std::string>();
            if (!text) return false;
            for (const std::string& pattern : values) {
                if (!xsdFullMatch(pattern, *text)) return false;
            }
        } else if (constraint == "length" || constraint == "maxLength" || constraint == "minLength") {
            const auto bound = pyIntFromString(value);
            if (!bound) return false;  // ValueError
            const auto length = static_cast<long long>(pyLen(pyStr(other)));
            if (constraint == "length" && length != *bound) return false;
            if (constraint == "maxLength" && length > *bound) return false;
            if (constraint == "minLength" && length < *bound) return false;
        } else if (constraint == "maxExclusive" || constraint == "maxInclusive" || constraint == "minExclusive"
                   || constraint == "minInclusive") {
            const auto number = pyFloat(other);
            const auto bound = pyFloatFromString(value);
            if (!number || !bound) return false;  // ValueError (TypeError treated alike)
            if (constraint == "maxExclusive" && *number >= *bound) return false;
            if (constraint == "maxInclusive" && *number > *bound) return false;
            if (constraint == "minExclusive" && *number <= *bound) return false;
            if (constraint == "minInclusive" && *number < *bound) return false;
        }
        // totalDigits / fractionDigits: not evaluated by IfcTester.
    }
    return true;
}

std::string Restriction::toString() const {
    std::string out = "{";
    bool first = true;
    for (const auto& [name, values] : options) {
        if (!first) out += ", ";
        first = false;
        out += "'" + name + "': ";
        if (values.size() == 1 && name != "enumeration") {
            out += "'" + values.front() + "'";
        } else {
            out += "[";
            for (std::size_t i = 0; i < values.size(); ++i) out += (i ? ", '" : "'") + values[i] + "'";
            out += "]";
        }
    }
    return out + "}";
}

Param Param::text(const std::string& value) {
    Param param;
    param.kind = Kind::Simple;
    param.simple = value;
    return param;
}

Param Param::of(const Restriction& restriction) {
    Param param;
    param.kind = Kind::Restriction;
    param.restriction = restriction;
    return param;
}

bool Param::matches(const Value& other) const {
    switch (kind) {
    case Kind::None: return other.is_null();
    case Kind::Simple: {
        const auto* text = other.get_if<std::string>();
        return text && *text == simple;
    }
    case Kind::Restriction: return restriction.equals(other);
    }
    return false;
}

std::string Param::toString() const {
    switch (kind) {
    case Kind::None: return "None";
    case Kind::Simple: return simple;
    case Kind::Restriction: return restriction.toString();
    }
    return {};
}

} // namespace bonsaiviewer::ports::ifctester
