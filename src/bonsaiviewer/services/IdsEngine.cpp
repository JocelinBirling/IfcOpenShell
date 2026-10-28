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
 ********************************************************************************/

#include "IdsEngine.h"

#include "pset.h" // helpers: get_psets

#include "../ifcparse/file.h"
#include "../ifcparse/schema.h"

#include <QSet>

#include <algorithm>
#include <map>
#include <set>

namespace bonsaiviewer::services::ids_engine {

namespace {

const char* const kSchemas[] = {"IFC2X3", "IFC4", "IFC4X3_ADD2"};
const QStringList kCardinalities = {"required", "optional", "prohibited"};
const QStringList kRelations = {"IFCRELAGGREGATES", "IFCRELASSIGNSTOGROUP", "IFCRELCONTAINEDINSPATIALSTRUCTURE",
                                "IFCRELNESTS", "IFCRELVOIDSELEMENT IFCRELFILLSELEMENT"};
const char* const kBoundKeys[] = {"minInclusive", "minExclusive", "maxInclusive", "maxExclusive"};
const char* const kAnySystem = ".*"; // a classification facet without a system

std::string s(const QString& text) {
    return text.toStdString();
}

QString q(const std::string& text) {
    return QString::fromStdString(text);
}

QString kindName(IdsValue::Kind kind) {
    switch (kind) {
    case IdsValue::Kind::Any: return "any";
    case IdsValue::Kind::Simple: return "simple";
    case IdsValue::Kind::Enumeration: return "enumeration";
    case IdsValue::Kind::Pattern: return "pattern";
    case IdsValue::Kind::Range: return "range";
    case IdsValue::Kind::Length: return "length";
    }
    return "any";
}

double number(const QString& text, const QString& label) {
    const auto value = it::pyFloatFromString(s(text.trimmed().replace(',', '.')));
    if (!value) throw UserError(s(QString("%1: '%2' is not a number").arg(label, text)));
    return *value;
}

// _format_number: "2" for 2.0, else the repr of the float.
QString formatNumber(double value) {
    if (std::isfinite(value) && value == std::floor(value) && std::abs(value) < 1e15) {
        return QString::number(static_cast<long long>(value));
    }
    return q(it::pyFloatRepr(value));
}

// ------------------------------------------------------------------ schema

std::vector<std::string> schemaNames(ifcopenshell::file* file) {
    if (file != nullptr && file->schema() != nullptr) return {file->schema()->name()};
    return {std::begin(kSchemas), std::end(kSchemas)};
}

const ifcopenshell::entity* declarationOf(const std::string& schema_name, const QString& class_name) {
    try {
        const auto* schema = ifcopenshell::schema_by_name(schema_name);
        return schema ? schema->declaration_by_name(s(class_name))->as_entity() : nullptr;
    } catch (...) {
        return nullptr; // the class (or the schema) does not exist
    }
}

// The class first, then its subtypes, depth first in name order.
QStringList walkSubtypes(const ifcopenshell::entity* declaration) {
    QStringList result;
    std::vector<const ifcopenshell::entity*> stack = {declaration};
    while (!stack.empty()) {
        const auto* current = stack.back();
        stack.pop_back();
        result << q(current->name());
        auto subtypes = current->subtypes();
        std::sort(subtypes.begin(), subtypes.end(),
                  [](const auto* a, const auto* b) { return a->name() > b->name(); });
        stack.insert(stack.end(), subtypes.begin(), subtypes.end());
    }
    return result;
}

void appendUnique(QStringList& list, const QString& value) {
    if (!list.contains(value)) list << value;
}

// class_names(class_name, include_subtypes, ifc_file)
QStringList classNames(const QString& class_name, bool include_subtypes, ifcopenshell::file* file) {
    QStringList names;
    for (const std::string& schema_name : schemaNames(file)) {
        const auto* declaration = declarationOf(schema_name, class_name);
        if (declaration == nullptr) continue;
        const QStringList found = include_subtypes ? walkSubtypes(declaration) : QStringList{q(declaration->name())};
        for (const QString& name : found) appendUnique(names, name);
    }
    if (names.isEmpty()) throw UserError(s("unknown IFC class: " + class_name));
    return names;
}

// predefined_types(class_name, ifc_file): the PredefinedType enumeration.
QStringList predefinedTypes(const QString& class_name, ifcopenshell::file* file) {
    QStringList values;
    for (const std::string& schema_name : schemaNames(file)) {
        const auto* declaration = declarationOf(schema_name, class_name);
        if (declaration == nullptr) continue;
        for (const auto* attribute : declaration->all_attributes()) {
            if (attribute->name() != "PredefinedType") continue;
            const ifcopenshell::parameter_type* type = attribute->type_of_attribute();
            const ifcopenshell::declaration* declared = nullptr;
            while (type != nullptr || declared != nullptr) {
                if (type != nullptr) {
                    const auto* named = type->as_named_type();
                    if (named == nullptr) break;
                    declared = named->declared_type();
                    type = nullptr;
                } else if (const auto* enumeration = declared->as_enumeration_type()) {
                    for (const auto& item : enumeration->enumeration_items()) appendUnique(values, q(item));
                    break;
                } else if (const auto* type_declaration = declared->as_type_declaration()) {
                    type = type_declaration->declared_type();
                    declared = nullptr;
                } else {
                    break;
                }
            }
        }
    }
    return values;
}

bool isStrictSubtype(const QString& name, const QString& other) {
    for (const char* schema_name : kSchemas) {
        const auto* declaration = declarationOf(schema_name, name);
        while (declaration != nullptr) {
            declaration = declaration->supertype();
            if (declaration != nullptr && q(declaration->name()) == other) return true;
        }
    }
    return false;
}

QString canonicalClass(const QString& name) {
    for (const char* schema_name : kSchemas) {
        if (const auto* declaration = declarationOf(schema_name, name)) return q(declaration->name());
    }
    return name;
}

// IDS compares the entity name in upper case: upper-case the letters of the
// pattern, but not the escapes (\d, \s...).
QString upperPattern(const QString& pattern) {
    QString out;
    bool escaped = false;
    for (const QChar c : pattern) {
        out += escaped ? c : c.toUpper();
        escaped = c == '\\' && !escaped;
    }
    return out;
}

// entity_names(spec_facet, ifc_file): the classes the facet names (a
// pattern is matched against the selectable classes; may be empty).
QStringList entityNames(const IdsValue& value, ifcopenshell::file* file) {
    QStringList names;
    switch (value.kind) {
    case IdsValue::Kind::Simple: names << value.simple.trimmed(); break;
    case IdsValue::Kind::Enumeration:
        for (const QString& v : value.values) names << v.trimmed();
        break;
    case IdsValue::Kind::Pattern: {
        const std::string pattern = s(upperPattern(value.pattern));
        QStringList matching;
        for (const QString& name : selectableClasses(file)) {
            if (it::xsdFullMatch(pattern, s(name.toUpper()))) matching << name;
        }
        return matching;
    }
    default:
        throw UserError(s("the IFC class cannot be a " + kindName(value.kind)));
    }
    names.removeAll(QString());
    if (names.isEmpty()) throw UserError("the entity facet needs an IFC class");
    return names;
}

QStringList coveredClasses(const QStringList& names, bool include_subtypes, ifcopenshell::file* file) {
    QStringList covered;
    for (const QString& name : names) {
        for (const QString& c : classNames(name, include_subtypes, file)) appendUnique(covered, c);
    }
    return covered;
}

QStringList classPredefinedTypes(const QStringList& names, ifcopenshell::file* file) {
    QStringList values;
    for (const QString& name : names) {
        for (const QString& v : predefinedTypes(name, file)) appendUnique(values, v);
    }
    return values;
}

// The `name` parameter of an entity / partOf facet. A pattern stays a
// pattern, unless the subtypes widen it to the classes it matches.
it::Param classParam(const IdsValue& value, bool include_subtypes, ifcopenshell::file* file) {
    const QStringList names = entityNames(value, file);
    if (value.kind == IdsValue::Kind::Pattern && !(include_subtypes && !names.isEmpty())) {
        it::Restriction restriction;
        restriction.add("pattern", s(upperPattern(value.pattern)));
        return it::Param::of(restriction);
    }
    QStringList upper;
    for (const QString& name : coveredClasses(names, include_subtypes, file)) upper << name.toUpper();
    if (upper.size() == 1) return it::Param::text(s(upper.front()));
    it::Restriction restriction;
    for (const QString& name : upper) restriction.add("enumeration", s(name));
    return it::Param::of(restriction);
}

it::Param predefinedParam(const IdsFacet& facet) {
    it::Param predefined = toParam(facet.predefined_type, "PredefinedType");
    if (predefined.isSimple()) {
        const QString upper = q(predefined.simple).toUpper();
        if (classPredefinedTypes(entityNames(facet.name, nullptr), nullptr).contains(upper)) {
            predefined = it::Param::text(s(upper));
        }
    }
    return predefined;
}

// fold_classes: an entity enumeration -> (classes, include_subtypes).
std::pair<QStringList, bool> foldClasses(const std::vector<std::string>& values) {
    QStringList canonical;
    for (const auto& value : values) appendUnique(canonical, canonicalClass(q(value)));
    const QSet<QString> listed(canonical.begin(), canonical.end());
    QStringList roots;
    for (const QString& name : canonical) {
        const bool has_listed_supertype = std::any_of(listed.begin(), listed.end(), [&](const QString& other) {
            return other != name && isStrictSubtype(name, other);
        });
        if (!has_listed_supertype) roots << name;
    }
    try {
        const QStringList covered = coveredClasses(roots, true, nullptr);
        if (QSet<QString>(covered.begin(), covered.end()) == listed && listed.size() > roots.size()) {
            return {roots, true};
        }
    } catch (const UserError&) {
    }
    return {canonical, false};
}

// The class of an entity / partOf facet, back to the editor.
void classFromParam(const it::Param& name, IdsFacet& facet) {
    facet.include_subtypes = false;
    if (name.isRestriction()) {
        if (const auto* values = name.restriction.option("enumeration")) {
            const auto [names, include_subtypes] = foldClasses(*values);
            facet.include_subtypes = include_subtypes;
            if (names.size() == 1) {
                facet.name = IdsValue::simpleValue(names.front());
            } else {
                facet.name.kind = IdsValue::Kind::Enumeration;
                facet.name.values = names;
            }
            return;
        }
        facet.name = fromParam(name);
        return;
    }
    facet.name = IdsValue::simpleValue(canonicalClass(q(name.simple)));
}

it::Cardinality cardinalityOf(const IdsFacet& facet, const QString& label) {
    if (!kCardinalities.contains(facet.cardinality)) {
        throw UserError(s(QString("%1: unknown cardinality '%2'").arg(label, facet.cardinality)));
    }
    return *it::cardinalityFromName(s(facet.cardinality));
}

QString valueText(const property_value& value) {
    if (const auto* b = value.get_if<bool>()) return *b ? "true" : "false";
    if (const auto* d = value.get_if<double>()) return formatNumber(*d);
    if (const auto* list = value.get_if<property_list>()) {
        QStringList items;
        for (const auto& item : *list) items << (item.get_if<std::string>() ? "'" + valueText(item) + "'" : valueText(item));
        return "[" + items.join(", ") + "]";
    }
    return q(it::pyStr(value));
}

bool isEmptyValue(const property_value& value) {
    const auto* text = value.get_if<std::string>();
    return value.is_null() || (text && text->empty());
}

std::vector<express::base> entityElements(ifcopenshell::file& file, const IdsFacet& entity) {
    IdsFacet facet = IdsFacet::entity({});
    facet.name = entity.name;
    facet.include_subtypes = entity.include_subtypes;
    it::Specification spec;
    spec.applicability = {toFacet(facet, 1, &file)};
    std::vector<express::base> elements;
    for (int id : select(file, spec)) elements.push_back(file.instance_by_id(id));
    return elements;
}

} // namespace

// ------------------------------------------------------------------ values

it::Param toParam(const IdsValue& value, const QString& label) {
    switch (value.kind) {
    case IdsValue::Kind::Any: return it::Param::none();
    case IdsValue::Kind::Simple: {
        const QString text = value.simple.trimmed();
        return text.isEmpty() ? it::Param::none() : it::Param::text(s(text));
    }
    case IdsValue::Kind::Enumeration: {
        it::Restriction restriction;
        for (const QString& v : value.values) {
            if (!v.trimmed().isEmpty()) restriction.add("enumeration", s(v.trimmed()));
        }
        if (restriction.options.empty()) throw UserError(s(label + ": the list of values is empty"));
        return it::Param::of(restriction);
    }
    case IdsValue::Kind::Pattern: {
        if (value.pattern.isEmpty()) throw UserError(s(label + ": the pattern is empty"));
        it::Restriction restriction;
        restriction.add("pattern", s(value.pattern));
        return it::Param::of(restriction);
    }
    case IdsValue::Kind::Range: {
        it::Restriction restriction;
        restriction.base = s(value.base.isEmpty() ? QString("double") : value.base);
        const QString low = value.min.trimmed();
        const QString high = value.max.trimmed();
        if (low.isEmpty() && high.isEmpty()) throw UserError(s(label + ": the range needs a minimum or a maximum"));
        if (!low.isEmpty() && !high.isEmpty() && number(low, label) > number(high, label)) {
            throw UserError(s(label + ": the minimum is greater than the maximum"));
        }
        const auto bound = [&](const QString& text) {
            const double v = number(text, label);
            if (restriction.base == "integer" && v == std::floor(v)) return QString::number(static_cast<long long>(v));
            return q(it::pyFloatRepr(v));
        };
        if (!low.isEmpty()) restriction.add(value.min_inclusive ? "minInclusive" : "minExclusive", s(bound(low)));
        if (!high.isEmpty()) restriction.add(value.max_inclusive ? "maxInclusive" : "maxExclusive", s(bound(high)));
        return it::Param::of(restriction);
    }
    case IdsValue::Kind::Length: {
        bool ok_low = true;
        bool ok_high = true;
        const QString low_text = value.min.trimmed();
        const QString high_text = value.max.trimmed();
        const std::optional<int> low = low_text.isEmpty() ? std::nullopt : std::optional<int>(low_text.toInt(&ok_low));
        const std::optional<int> high = high_text.isEmpty() ? std::nullopt : std::optional<int>(high_text.toInt(&ok_high));
        if (!ok_low || !ok_high) throw UserError(s(label + ": a length is a whole number of characters"));
        if (!low && !high) throw UserError(s(label + ": the length needs a minimum or a maximum"));
        if (low && high && *low > *high) throw UserError(s(label + ": the minimum length is greater than the maximum"));
        it::Restriction restriction;
        if (low && low == high) {
            restriction.add("length", std::to_string(*low));
        } else {
            if (low) restriction.add("minLength", std::to_string(*low));
            if (high) restriction.add("maxLength", std::to_string(*high));
        }
        return it::Param::of(restriction);
    }
    }
    return it::Param::none();
}

IdsValue fromParam(const it::Param& param) {
    IdsValue value;
    if (param.isNone() || (param.isSimple() && param.simple.empty())) return value;
    if (param.isSimple()) return IdsValue::simpleValue(q(param.simple));
    const it::Restriction& restriction = param.restriction;
    if (const auto* values = restriction.option("enumeration")) {
        value.kind = IdsValue::Kind::Enumeration;
        for (const auto& v : *values) value.values << q(v);
        return value;
    }
    if (const auto* patterns = restriction.option("pattern")) {
        value.kind = IdsValue::Kind::Pattern;
        value.pattern = patterns->empty() ? QString() : q(patterns->front());
        return value;
    }
    bool has_bound = false;
    for (const char* key : kBoundKeys) has_bound = has_bound || restriction.option(key) != nullptr;
    if (has_bound) {
        value.kind = IdsValue::Kind::Range;
        value.base = restriction.base.empty() ? QString("double") : q(restriction.base);
        for (const char* key : kBoundKeys) {
            const auto* bounds = restriction.option(key);
            if (bounds == nullptr || bounds->empty()) continue;
            const auto number = it::pyFloatFromString(bounds->front());
            const QString text = number ? formatNumber(*number) : q(bounds->front());
            const bool inclusive = QString(key).endsWith("Inclusive");
            if (QString(key).startsWith("min")) {
                value.min = text;
                value.min_inclusive = inclusive;
            } else {
                value.max = text;
                value.max_inclusive = inclusive;
            }
        }
        return value;
    }
    const auto first = [&](const char* key) {
        const auto* bounds = restriction.option(key);
        if (bounds == nullptr || bounds->empty()) return QString();
        const auto number = it::pyFloatFromString(bounds->front());
        return number ? QString::number(static_cast<long long>(*number)) : QString();
    };
    if (restriction.option("length")) {
        value.kind = IdsValue::Kind::Length;
        value.min = value.max = first("length");
        return value;
    }
    if (restriction.option("minLength") || restriction.option("maxLength")) {
        value.kind = IdsValue::Kind::Length;
        value.min = first("minLength");
        value.max = first("maxLength");
        return value;
    }
    // totalDigits / fractionDigits: IfcTester does not evaluate them.
    throw UserError("unsupported restriction: " + restriction.toString());
}

// ------------------------------------------------------------------ facets

namespace {

it::Facet toFacetFields(const IdsFacet& facet, int index, ifcopenshell::file* file);

std::optional<std::string> optionalText(const QString& text) {
    return text.trimmed().isEmpty() ? std::nullopt : std::optional<std::string>(s(text.trimmed()));
}

} // namespace

it::Facet toFacet(const IdsFacet& facet, int index, ifcopenshell::file* file) {
    it::Facet result = toFacetFields(facet, index, file);
    if (facet.type != IdsFacet::Type::Entity) result.uri = optionalText(facet.uri);
    result.instructions = optionalText(facet.instructions);
    return result;
}

namespace {

it::Facet toFacetFields(const IdsFacet& facet, int index, ifcopenshell::file* file) {
    switch (facet.type) {
    case IdsFacet::Type::Entity: {
        it::Facet result = it::Facet::make(it::FacetType::Entity);
        result.name = classParam(facet.name, facet.include_subtypes, file);
        result.predefinedType = predefinedParam(facet);
        return result;
    }
    case IdsFacet::Type::Property: {
        const QString label = QString("property facet %1").arg(index);
        it::Facet result = it::Facet::make(it::FacetType::Property);
        result.propertySet = toParam(facet.pset, label + " (property set)");
        result.baseName = toParam(facet.property, label + " (property)");
        if (result.propertySet.isNone() || result.baseName.isNone()) {
            throw UserError(s(label + ": both the property set and the property name are needed"));
        }
        result.value = toParam(facet.value, label);
        if (!facet.data_type.trimmed().isEmpty()) result.dataType = s(facet.data_type.trimmed().toUpper());
        result.cardinality = cardinalityOf(facet, label);
        return result;
    }
    case IdsFacet::Type::Attribute: {
        const QString label = QString("attribute facet %1").arg(index);
        it::Facet result = it::Facet::make(it::FacetType::Attribute);
        result.name = toParam(facet.property, label + " (attribute)");
        if (result.name.isNone()) throw UserError(s(label + ": the attribute name is needed"));
        result.value = toParam(facet.value, label);
        result.cardinality = cardinalityOf(facet, label);
        return result;
    }
    case IdsFacet::Type::Classification: {
        const QString label = QString("classification facet %1").arg(index);
        it::Facet result = it::Facet::make(it::FacetType::Classification);
        result.system = toParam(facet.system, label + " (system)");
        if (result.system.isNone()) {
            it::Restriction any;
            any.add("pattern", kAnySystem);
            result.system = it::Param::of(any);
        }
        result.value = toParam(facet.value, label);
        result.cardinality = cardinalityOf(facet, label);
        return result;
    }
    case IdsFacet::Type::Material: {
        const QString label = QString("material facet %1").arg(index);
        it::Facet result = it::Facet::make(it::FacetType::Material);
        result.value = toParam(facet.value, label);
        result.cardinality = cardinalityOf(facet, label);
        return result;
    }
    case IdsFacet::Type::PartOf: {
        const QString label = QString("part of facet %1").arg(index);
        it::Facet result = it::Facet::make(it::FacetType::PartOf);
        const QString relation = facet.relation.trimmed().toUpper();
        if (!relation.isEmpty() && !kRelations.contains(relation)) {
            throw UserError(s(QString("%1: unknown relation '%2'").arg(label, relation)));
        }
        if (!relation.isEmpty()) result.relation = s(relation);
        result.name = classParam(facet.name, facet.include_subtypes, file);
        result.predefinedType = predefinedParam(facet);
        result.cardinality = cardinalityOf(facet, label);
        return result;
    }
    }
    throw UserError(s(QString("facet %1: unsupported facet type").arg(index)));
}

} // namespace

IdsFacet fromFacet(const it::Facet& facet) {
    IdsFacet result;
    result.cardinality = q(it::cardinalityName(facet.cardinality));
    switch (facet.type) {
    case it::FacetType::Entity:
        result = IdsFacet::ofType(IdsFacet::Type::Entity);
        classFromParam(facet.name, result);
        result.predefined_type = fromParam(facet.predefinedType);
        break;
    case it::FacetType::PartOf:
        result.type = IdsFacet::Type::PartOf;
        classFromParam(facet.name, result);
        result.predefined_type = fromParam(facet.predefinedType);
        result.relation = facet.relation ? q(*facet.relation) : QString();
        break;
    case it::FacetType::Property:
        result.type = IdsFacet::Type::Property;
        result.pset = fromParam(facet.propertySet);
        result.property = fromParam(facet.baseName);
        result.value = fromParam(facet.value);
        result.data_type = facet.dataType ? q(*facet.dataType).toUpper() : QString();
        break;
    case it::FacetType::Attribute:
        result.type = IdsFacet::Type::Attribute;
        result.property = fromParam(facet.name);
        result.value = fromParam(facet.value);
        break;
    case it::FacetType::Classification: {
        result.type = IdsFacet::Type::Classification;
        const auto* pattern = facet.system.isRestriction() ? facet.system.restriction.option("pattern") : nullptr;
        const bool any = pattern && facet.system.restriction.options.size() == 1 && *pattern == std::vector<std::string>{kAnySystem};
        result.system = any ? IdsValue() : fromParam(facet.system);
        result.value = fromParam(facet.value);
        break;
    }
    case it::FacetType::Material:
        result.type = IdsFacet::Type::Material;
        result.value = fromParam(facet.value);
        break;
    }
    result.uri = facet.uri ? q(*facet.uri) : QString();
    result.instructions = facet.instructions ? q(*facet.instructions) : QString();
    return result;
}

it::Specification buildSpecification(const QString& name, const std::vector<IdsFacet>& facets,
                                      ifcopenshell::file* file) {
    const auto entities = std::count_if(facets.begin(), facets.end(),
                                        [](const IdsFacet& f) { return f.type == IdsFacet::Type::Entity; });
    if (entities != 1) throw UserError("a selection needs exactly one entity facet");
    it::Specification spec;
    spec.name = s(name);
    int index = 0;
    for (const IdsFacet& facet : facets) {
        ++index;
        if (facet.type == IdsFacet::Type::Entity) {
            spec.applicability.push_back(toFacet(facet, index, file));
        } else {
            spec.requirements.push_back(toFacet(facet, index, file));
        }
    }
    spec.requirement_outcomes.assign(spec.requirements.size(), it::FacetOutcome());
    return spec;
}

QString buildXml(const QString& name, const std::vector<IdsFacet>& facets) {
    it::Ids document;
    document.setInfo("title", s(name));
    document.specifications = {buildSpecification(name, facets)};
    return QString::fromUtf8(it::idsToXml(document));
}

std::vector<IdsFacet> facetsOf(const it::Specification& specification) {
    std::vector<IdsFacet> facets;
    for (const auto* clause : {&specification.applicability, &specification.requirements}) {
        for (const it::Facet& facet : *clause) facets.push_back(fromFacet(facet));
    }
    return facets;
}

std::vector<IdsFacet> parseXml(const QString& ids_xml, QString* name) {
    std::string error;
    const auto document = it::idsFromXml(ids_xml.toUtf8(), &error);
    if (!document) throw UserError("the IDS document cannot be read: " + error);
    if (document->specifications.empty()) throw UserError("the IDS document has no specification");
    const it::Specification& spec = document->specifications.front();
    if (name != nullptr) *name = q(spec.name);
    return facetsOf(spec);
}

QString facetsToClipboard(const std::vector<IdsFacet>& facets) {
    std::vector<it::Facet> converted;
    int index = 0;
    for (const IdsFacet& facet : facets) converted.push_back(toFacet(facet, ++index));
    return QString::fromUtf8(it::facetsToXml(converted));
}

std::vector<IdsFacet> facetsFromClipboard(const QString& text) {
    const auto facets = it::facetsFromXml(text.trimmed().toUtf8());
    if (!facets) throw UserError("the clipboard holds no IDS facet");
    std::vector<IdsFacet> result;
    for (const it::Facet& facet : *facets) result.push_back(fromFacet(facet));
    return result;
}

// ---------------------------------------------------------------- selection

std::vector<int> select(ifcopenshell::file& file, const it::Specification& specification,
                        const std::optional<std::vector<int>>& limit) {
    // Broadphase applicability, like Specification.validate.
    std::optional<std::vector<express::base>> elements;
    for (const it::Facet& facet : specification.applicability) {
        elements = facet.filter(file, elements ? &*elements : nullptr);
    }
    std::set<int> limit_set;
    if (limit) limit_set.insert(limit->begin(), limit->end());
    if (!elements) {
        elements.emplace();
        if (limit) {
            for (int id : limit_set) {
                try {
                    if (auto element = file.instance_by_id(id)) elements->push_back(element);
                } catch (...) {
                }
            }
        } else {
            *elements = file.instances_by_type("IfcProduct");
        }
    } else if (limit) {
        std::vector<express::base> kept;
        for (const auto& e : *elements) {
            if (limit_set.count(static_cast<int>(e.id()))) kept.push_back(e);
        }
        elements = std::move(kept);
    }
    // Every facet must pass; the applicability is re-tested since some
    // broadphase filters only over-approximate.
    std::set<int> selected;
    for (const auto& element : *elements) {
        const auto passes = [&](const it::Facet& facet) { return static_cast<bool>(facet(element)); };
        if (std::all_of(specification.applicability.begin(), specification.applicability.end(), passes) &&
            std::all_of(specification.requirements.begin(), specification.requirements.end(), passes)) {
            selected.insert(static_cast<int>(element.id()));
        }
    }
    return {selected.begin(), selected.end()};
}

// ------------------------------------------------------------ introspection

QStringList selectableClasses(ifcopenshell::file* file) {
    QSet<QString> names;
    for (const std::string& schema_name : schemaNames(file)) {
        if (const auto* root = declarationOf(schema_name, "IfcObjectDefinition")) {
            for (const QString& name : walkSubtypes(root)) names.insert(name);
        }
    }
    QStringList sorted(names.begin(), names.end());
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

IdsBridge::ClassInfo classInfo(ifcopenshell::file* file, const IdsValue& name, bool include_subtypes) {
    IdsBridge::ClassInfo info;
    const QStringList names = entityNames(name, file);
    info.classes = coveredClasses(names, include_subtypes, file);
    info.predefined_types = classPredefinedTypes(names, file);
    if (names.isEmpty()) info.warning = "the class pattern matches no IFC class: the layer will select nothing";
    if (file != nullptr) {
        IdsFacet entity = IdsFacet::entity({});
        entity.name = name;
        entity.include_subtypes = include_subtypes;
        info.count = static_cast<int>(entityElements(*file, entity).size());
    }
    return info;
}

void properties(ifcopenshell::file& file, const IdsFacet& entity, IdsBridge::PropertyMap& psets,
                IdsBridge::SampleMap& samples) {
    for (const auto& element : entityElements(file, entity)) {
        for (const auto& [pset_name, pset_properties] : get_psets(element)) {
            auto& target = psets[q(pset_name)];
            auto& target_samples = samples[q(pset_name)];
            for (const auto& [property_name, value] : pset_properties) {
                if (property_name == "id") continue;
                const QString key = q(property_name);
                target[key] = target.value(key, 0) + 1;
                QStringList& shown = target_samples[key];
                if (!isEmptyValue(value) && shown.size() < 3) appendUnique(shown, valueText(value));
            }
        }
    }
}

IdsBridge::ValueStats values(ifcopenshell::file& file, const IdsFacet& entity, const QString& pset,
                             const QString& property) {
    IdsBridge::ValueStats stats;
    std::map<QString, int> counts;
    std::set<QString> kinds;
    for (const auto& element : entityElements(file, entity)) {
        const auto all = get_psets(element);
        const auto found = all.find(s(pset));
        const property_value* value = nullptr;
        if (found != all.end()) {
            const auto prop = found->second.find(s(property));
            if (prop != found->second.end()) value = &prop->second;
        }
        if (value == nullptr || isEmptyValue(*value)) {
            ++stats.missing;
            continue;
        }
        kinds.insert(value->get_if<bool>()           ? "boolean"
                     : value->get_if<std::int64_t>() ? "integer"
                     : value->get_if<double>()       ? "double"
                                                     : "string");
        ++counts[valueText(*value)];
    }
    const auto only = [&](std::initializer_list<const char*> allowed) {
        return !kinds.empty() && std::all_of(kinds.begin(), kinds.end(), [&](const QString& k) {
            return std::any_of(allowed.begin(), allowed.end(), [&](const char* a) { return k == a; });
        });
    };
    stats.base = only({"integer"}) ? "integer" : only({"integer", "double"}) ? "double" : only({"boolean"}) ? "boolean" : "string";
    std::vector<std::pair<QString, int>> sorted(counts.begin(), counts.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    });
    for (const auto& [value, count] : sorted) stats.values.append({value, count});
    return stats;
}

} // namespace bonsaiviewer::services::ids_engine
