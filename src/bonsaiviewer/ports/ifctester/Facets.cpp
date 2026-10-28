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

// PORT — see Facets.h. Each block quotes the Python it mirrors. Where the
// Python would raise (assert, AttributeError, KeyError on odd files), the
// port fails the facet or skips the unit conversion instead of aborting.

#include "Facets.h"

#include "../ifcopenshell_util/Classification.h"
#include "../ifcopenshell_util/Element.h"
#include "../ifcopenshell_util/Unit.h"

#include "../../../helpers/unit_convert.h"
#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace bonsaiviewer::ports::ifctester {

namespace util = ifcopenshell_util;

namespace {

std::string upper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::toupper(c); });
    return text;
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

std::string removeSuffix(const std::string& text, const std::string& suffix) {
    if (text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return text.substr(0, text.size() - suffix.size());
    }
    return text;
}

void replaceAll(std::string& text, const std::string& from, const std::string& to) {
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size())) {
        text.replace(at, from.size(), to);
    }
}

Value textValue(const std::optional<std::string>& text) {
    return text ? Value(*text) : Value();
}

// Python repr() of a value, for the messages: str(list) shows its items' repr.
std::string pyRepr(const Value& value) {
    if (isString(value)) return "'" + pyStr(value) + "'";
    if (const auto* list = value.get_if<property_list>()) {
        std::string out = "[";
        for (std::size_t i = 0; i < list->size(); ++i) out += (i ? ", " : "") + pyRepr((*list)[i]);
        return out + "]";
    }
    return pyStr(value);
}

std::string reprList(const std::vector<Value>& values) {
    return pyRepr(Value(property_list(values.begin(), values.end())));
}

std::string reprSet(const std::vector<Value>& values) {
    if (values.empty()) return "set()";
    std::string out = "{";
    for (std::size_t i = 0; i < values.size(); ++i) out += (i ? ", " : "") + pyRepr(values[i]);
    return out + "}";
}

Result pass() {
    return {true, {}};
}

Result fail(const std::string& reason) {
    return {false, reason};
}

// `if self.cardinality == "prohibited": return XResult(not is_pass, {"type": "PROHIBITED"})`
Result finish(const Facet& facet, bool is_pass, const std::string& reason, const char* prohibited_message) {
    if (facet.cardinality == Cardinality::Prohibited) return {!is_pass, is_pass ? prohibited_message : ""};
    return {is_pass, is_pass ? "" : reason};
}

std::vector<Entity> byType(ifcopenshell::file& file, const char* ifc_class) {
    try {
        return file.instances_by_type(ifc_class);
    } catch (...) {
        return {};  // The class does not exist in this schema.
    }
}

// ------------------------------------------------------------------ Entity

// derived_occurrence_class = element_type.is_a().upper().removesuffix("TYPE").removesuffix("STYLE")
std::string derivedOccurrenceClass(const Entity& element_type) {
    return removeSuffix(removeSuffix(upper(util::className(element_type)), "TYPE"), "STYLE");
}

std::vector<Entity> entityFilter(const Facet& facet, ifcopenshell::file& file) {
    const auto* schema = file.schema();
    if (!schema) return {};
    std::vector<Entity> results;
    // ifc_file.by_type(name, include_subtypes=False) for every class matching
    // self.name (str or Restriction).
    for (const auto* entity : schema->entities()) {
        if (!facet.name.matches(Value(entity->name_uc()))) continue;
        const auto instances = file.instances_by_type_excl_subtypes(entity);
        results.insert(results.end(), instances.begin(), instances.end());
    }
    if (schema->name() == "IFC2X3") {
        // Occurrences typed by an IfcTypeProduct whose derived class matches.
        for (const Entity& element_type : byType(file, "IfcTypeProduct")) {
            if (!facet.name.matches(Value(derivedOccurrenceClass(element_type)))) continue;
            const auto types = util::get_types(element_type);
            results.insert(results.end(), types.begin(), types.end());
        }
        // results = list(set(results))
        std::sort(results.begin(), results.end());
        results.erase(std::unique(results.begin(), results.end()), results.end());
    }
    if (facet.predefinedType.truthy()) {
        std::vector<Entity> kept;
        for (const Entity& result : results) {
            if (facet(result)) kept.push_back(result);
        }
        return kept;
    }
    return results;
}

Result entityCall(const Facet& facet, const Entity& inst) {
    const std::string inst_class = upper(util::className(inst));
    bool is_pass = facet.name.matches(Value(inst_class));
    std::string reason;
    const Entity element_type = !is_pass && util::schemaName(inst) == "IFC2X3" ? util::get_type(inst) : Entity();
    if (element_type && element_type != inst) {
        const std::string derived = derivedOccurrenceClass(element_type);
        is_pass = facet.name.matches(Value(derived));
        reason = "The entity class \"" + derived + "\" does not meet the required IFC class";
    } else if (!is_pass) {
        reason = "The entity class \"" + inst_class + "\" does not meet the required IFC class";
    }

    if (is_pass && facet.predefinedType.truthy()) {
        std::optional<std::string> predefined_type;
        if (facet.predefinedType.matches(Value("USERDEFINED"))) {
            is_pass = util::is_userdefined_type(inst);
            if (!is_pass) predefined_type = util::get_predefined_type(inst);
        } else {
            predefined_type = util::get_predefined_type(inst);
            is_pass = facet.predefinedType.matches(textValue(predefined_type));
        }
        if (!is_pass) {
            reason = "The predefined type \"" + predefined_type.value_or("None") + "\" does not meet the required type";
        }
    }
    return is_pass ? pass() : fail(reason);
}

// --------------------------------------------------------------- Attribute

std::vector<Entity> attributeFilter(const Facet& facet, ifcopenshell::file& file) {
    // Every instance whose class has an attribute named self.name.
    const auto* schema = file.schema();
    if (!schema) return {};
    std::vector<Entity> results;
    for (const auto* entity : schema->entities()) {
        bool has_attribute = false;
        for (const auto* attribute : entity->all_attributes()) {
            if (facet.name.matches(Value(attribute->name()))) {
                has_attribute = true;
                break;
            }
        }
        if (!has_attribute) continue;
        const auto instances = file.instances_by_type_excl_subtypes(entity);
        results.insert(results.end(), instances.begin(), instances.end());
    }
    return results;
}

Result attributeCall(const Facet& facet, const Entity& inst) {
    struct Raw {
        Value value;
        bool is_logical = false;
    };
    std::vector<Raw> values;
    const auto* entity = inst.declaration().as_entity();
    const auto read = [&](const std::string& attribute_name) {
        const auto index = entity->attribute_index(attribute_name);
        const auto raw = inst.get_attribute_value(static_cast<std::size_t>(index));
        values.push_back({fromAttribute(raw), !raw.isNull() && raw.type() == ifcopenshell::Argument_LOGICAL});
    };
    if (entity) {
        if (facet.name.isSimple()) {
            if (util::attributeCategory(inst, facet.name.simple) == 1) read(facet.name.simple);
        } else {
            for (const std::string& attribute_name : util::forwardAttributeNames(inst)) {
                if (facet.name.matches(Value(attribute_name))) read(attribute_name);
            }
        }
    }

    bool is_pass = !values.empty();
    std::string reason;
    if (!is_pass) {
        if (facet.cardinality == Cardinality::Optional) return pass();
        reason = "The required attribute did not exist";
    }

    std::vector<Value> kept;
    if (is_pass) {
        for (const Raw& raw : values) {
            const Value& value = raw.value;
            bool is_empty = isNone(value);
            if (const auto* text = value.get_if<std::string>()) {
                is_empty = text->empty() || (raw.is_logical && *text == "UNKNOWN");
            }
            if (const auto* list = value.get_if<property_list>()) is_empty = list->empty();
            if (!is_empty) kept.push_back(value);
        }
        if (kept.empty()) {
            is_pass = false;
            const std::string actual = values.size() > 1 ? reprList([&] {
                std::vector<Value> all;
                for (const Raw& raw : values) all.push_back(raw.value);
                return all;
            }())
                                                         : pyStr(values.front().value);
            reason = "The attribute value \"" + actual + "\" is empty";
        }
    }

    if (is_pass && facet.value.truthy()) {
        for (const Value& value : kept) {
            bool ok = true;
            if (isEntity(value)) {
                ok = false;
            } else if (facet.value.isSimple() && isString(value)) {
                ok = *value.get_if<std::string>() == facet.value.simple;
            } else if (facet.value.isSimple()) {
                const Value cast_value = castToValue(facet.value.simple, value);
                const auto* number = value.get_if<double>();
                const auto* cast_number = cast_value.get_if<double>();
                ok = number && cast_number ? isX(*number, *cast_number) : pyEq(value, cast_value);
            } else {
                ok = facet.value.matches(value);
            }
            if (!ok) {
                is_pass = false;
                reason = "The attribute value \"" + pyStr(value) + "\" does not match the requirement";
                break;
            }
        }
    }
    return finish(facet, is_pass, reason, "The attribute value should not have met the requirement");
}

// ---------------------------------------------------------- Classification

Result classificationCall(const Facet& facet, const Entity& inst) {
    const auto leaf_references = util::get_references(inst);
    util::EntitySet references = leaf_references;
    for (const Entity& leaf_reference : leaf_references) {
        for (const Entity& reference : util::get_inherited_references(leaf_reference)) references.insert(reference);
    }

    bool is_pass = !references.empty();
    std::string reason;
    if (!is_pass) {
        if (facet.cardinality == Cardinality::Optional) return pass();
        reason = "The entity has no classification";
    }

    if (is_pass && facet.value.truthy()) {
        // getattr(r, "Identification", getattr(r, "ItemReference", None))
        std::vector<Value> values;
        for (const Entity& reference : references) {
            values.push_back(util::attributeCategory(reference, "Identification") == 1
                                 ? textValue(util::forwardString(reference, "Identification"))
                                 : textValue(util::forwardString(reference, "ItemReference")));
        }
        is_pass = std::any_of(values.begin(), values.end(), [&](const Value& v) { return facet.value.matches(v); });
        if (!is_pass) reason = "The references \"" + reprList(values) + "\" do not match the requirements";
    }

    if (is_pass) {
        std::vector<Value> systems;
        for (const Entity& reference : references) {
            if (const Entity classification = util::get_classification(reference)) {
                systems.push_back(textValue(util::forwardString(classification, "Name")));
            }
        }
        is_pass = std::any_of(systems.begin(), systems.end(), [&](const Value& s) { return facet.system.matches(s); });
        if (!is_pass) reason = "The systems \"" + reprList(systems) + "\" do not match the requirements";
    }
    return finish(facet, is_pass, reason, "The classification should not have met the requirement");
}

// ------------------------------------------------------------------ PartOf

// PartOf.get_parent(element): get_parent, else the group of the element.
Entity partOfParent(const Entity& element) {
    Entity parent = util::get_parent(element);
    if (!parent) {
        for (const Entity& rel : util::inverse(element, "HasAssignments")) {
            if (util::isA(rel, "IfcRelAssignsToGroup")) return util::forwardEntity(rel, "RelatingGroup");
        }
    }
    return parent;
}

std::string reprAncestors(const std::vector<std::string>& ancestors) {
    std::vector<Value> values(ancestors.begin(), ancestors.end());
    return reprList(values);
}

// Walks `next` up from `start` until a class matching the facet: the
// aggregate / nest / parent chains of PartOf.__call__.
template <typename Next>
bool walkChain(const Facet& facet, Entity current, Next next, std::string& reason) {
    std::vector<std::string> ancestors;
    while (current) {
        const std::string current_class = upper(util::className(current));
        ancestors.push_back(current_class);
        if (facet.name.matches(Value(current_class))) {
            if (facet.predefinedType.truthy()) {
                const auto predefined_type = util::get_predefined_type(current);
                ancestors.back() += "." + predefined_type.value_or("None");
                if (facet.predefinedType.matches(textValue(predefined_type))) return true;
            } else {
                return true;
            }
            break;
        }
        current = next(current);
    }
    reason = "The entity has a relationship with incorrect entities: \"" + reprAncestors(ancestors) + "\"";
    return false;
}

// The IFCRELASSIGNSTOGROUP / IFCRELCONTAINEDINSPATIALSTRUCTURE /
// IFCRELVOIDSELEMENT checks on one related entity. `check_type_after_name`
// is false for groups, where Python checks the type even after a wrong name.
bool checkRelated(const Facet& facet, const Entity& related, bool check_type_after_name, std::string& reason) {
    bool is_pass = true;
    const std::string related_class = upper(util::className(related));
    if (!facet.name.matches(Value(related_class))) {
        is_pass = false;
        reason = "The entity has a relationship with incorrect entities: \"" + related_class + "\"";
    }
    if ((is_pass || !check_type_after_name) && facet.predefinedType.truthy()) {
        const auto predefined_type = util::get_predefined_type(related);
        if (!facet.predefinedType.matches(textValue(predefined_type))) {
            is_pass = false;
            reason = "The entity has a relationship with incorrect predefined type: \"" +
                     predefined_type.value_or("None") + "\"";
        }
    }
    return is_pass;
}

Result partOfCall(const Facet& facet, const Entity& inst) {
    const std::string no_value = "The entity has no relationship";
    std::string reason;
    bool is_pass = false;
    const std::string relation = facet.relation.value_or("");
    if (relation.empty()) {
        is_pass = walkChain(facet, partOfParent(inst), partOfParent, reason);
    } else if (relation == "IFCRELAGGREGATES") {
        const Entity aggregate = util::get_aggregate(inst);
        is_pass = static_cast<bool>(aggregate);
        if (!is_pass) reason = no_value;
        if (is_pass && facet.name.truthy()) {
            is_pass = walkChain(facet, aggregate, [](const Entity& e) { return util::get_aggregate(e); }, reason);
        }
    } else if (relation == "IFCRELASSIGNSTOGROUP") {
        Entity group;
        for (const Entity& rel : util::inverse(inst, "HasAssignments")) {
            if (util::isA(rel, "IfcRelAssignsToGroup")) {
                group = util::forwardEntity(rel, "RelatingGroup");
                break;
            }
        }
        is_pass = static_cast<bool>(group);
        if (!is_pass) reason = no_value;
        if (is_pass && facet.name.truthy()) is_pass = checkRelated(facet, group, false, reason);
    } else if (relation == "IFCRELCONTAINEDINSPATIALSTRUCTURE") {
        const Entity container = util::get_container(inst);
        is_pass = static_cast<bool>(container);
        if (!is_pass) reason = no_value;
        if (is_pass && facet.name.truthy()) is_pass = checkRelated(facet, container, true, reason);
    } else if (relation == "IFCRELNESTS") {
        const Entity nest = util::get_nest(inst);
        is_pass = static_cast<bool>(nest);
        if (!is_pass) reason = no_value;
        if (is_pass && facet.name.truthy()) {
            is_pass = walkChain(facet, nest, [](const Entity& e) { return util::get_nest(e); }, reason);
        }
    } else if (relation == "IFCRELVOIDSELEMENT IFCRELFILLSELEMENT") {
        Entity building_element;
        if (util::isA(inst, "IfcOpeningElement")) {
            building_element = util::get_voided_element(inst);
        } else if (const Entity opening = util::get_filled_void(inst)) {
            building_element = util::get_voided_element(opening);
        }
        is_pass = static_cast<bool>(building_element);
        if (!is_pass) reason = no_value;
        if (is_pass && facet.name.truthy()) is_pass = checkRelated(facet, building_element, true, reason);
    } else {
        reason = no_value;  // Python: assert False, self.relation
    }
    return finish(facet, is_pass, reason, "The relationship should not have met the requirement");
}

// ---------------------------------------------------------------- Property

// Property.get_properties(pset); nullopt for an IfcPreDefinedPropertySet,
// whose "properties" are plain attributes (always accepted).
std::optional<std::vector<Entity>> propertiesOf(const Entity& pset) {
    if (util::isA(pset, "IfcPropertySet")) return util::forwardEntities(pset, "HasProperties");
    if (util::isA(pset, "IfcElementQuantity")) return util::forwardEntities(pset, "Quantities");
    if (util::isA(pset, "IfcMaterialProperties") || util::isA(pset, "IfcProfileProperties")) {
        return util::forwardEntities(pset, "Properties");
    }
    if (util::isA(pset, "IfcPreDefinedPropertySet")) return std::nullopt;
    return std::vector<Entity>();
}

bool isConversionUnit(const std::string& name) {
    return SI_CONVERSIONS.count(to_lower(name)) != 0;
}

// ifcopenshell.util.unit.convert(value, unit.Prefix, unit.Name, output_prefix,
// si_type_names[unit.UnitType]); unchanged when Python would leave the value
// as is (no prefix, no conversion based unit) or would raise.
Value convertValue(const Value& value, const Entity& unit, const std::optional<std::string>& output_prefix) {
    if (!unit) return value;
    const auto name = util::unitName(unit);
    const auto unit_type = util::unitType(unit);
    if (!name || !unit_type) return value;
    const auto si = util::si_type_names().find(*unit_type);
    if (si == util::si_type_names().end()) return value;
    const auto prefix = util::unitPrefix(unit);
    double number = 0.0;
    if (const auto* d = value.get_if<double>()) {
        number = *d;
    } else if (const auto* i = value.get_if<std::int64_t>()) {
        number = static_cast<double>(*i);
    } else {
        return value;
    }
    const bool unchanged = !isConversionUnit(*name) && !prefix && !isConversionUnit(si->second) && !output_prefix;
    if (unchanged) return value;
    return util::convert(number, prefix, *name, output_prefix, si->second);
}

Value convertList(const Value& value, const Entity& unit) {
    const auto* list = value.get_if<property_list>();
    if (!list || !unit) return value;
    property_list converted;
    for (const Value& item : *list) converted.push_back(convertValue(item, unit, std::nullopt));
    return converted;
}

std::string dataTypeReason(const std::string& actual, const Facet& facet) {
    return "The property's data type \"" + actual + "\" does not match the required data type of \"" +
           facet.dataType.value_or("None") + "\"";
}

bool dataTypeDiffers(const Facet& facet, const std::string& data_type) {
    return facet.dataType && !facet.dataType->empty() && lower(data_type) != lower(*facet.dataType);
}

std::string valueReason(const Value& actual) {
    if (const auto* list = actual.get_if<property_list>()) {
        if (list->size() == 1) return "The property value \"" + pyStr(list->front()) + "\" does not match the requirements";
        return "The property values \"" + pyRepr(actual) + "\" do not match the requirements";
    }
    return "The property value \"" + pyStr(actual) + "\" does not match the requirements";
}

// The value test at the end of Property.__call__ for one property value.
bool propertyValueMatches(const Facet& facet, const Value& value) {
    const auto* list = value.get_if<property_list>();
    if (facet.value.isSimple() && isString(value)) return *value.get_if<std::string>() == facet.value.simple;
    if (facet.value.isSimple() && list) {
        if (list->empty()) return false;
        const Value cast_value = castToValue(facet.value.simple, list->front());
        return std::any_of(list->begin(), list->end(), [&](const Value& v) { return pyEq(v, cast_value); });
    }
    if (!facet.value.isSimple() && list) {
        return std::any_of(list->begin(), list->end(), [&](const Value& v) { return facet.value.matches(v); });
    }
    if (facet.value.isSimple()) {
        const Value cast_value = castToValue(facet.value.simple, value);
        const auto* number = value.get_if<double>();
        const auto* cast_number = cast_value.get_if<double>();
        return number && cast_number ? isX(*number, *cast_number) : pyEq(value, cast_value);
    }
    return facet.value.matches(value);
}

std::vector<Entity> propertyFilter(ifcopenshell::file& file) {
    std::vector<Entity> results = byType(file, "IfcObjectDefinition");
    if (file.schema() && file.schema()->name() == "IFC2X3") return results;
    for (const char* ifc_class : {"IfcMaterialDefinition", "IfcProfileDef"}) {
        const auto more = byType(file, ifc_class);
        results.insert(results.end(), more.begin(), more.end());
    }
    return results;
}

Result propertyCall(const Facet& facet, const Entity& inst) {
    // psets: name -> property map (with its "id"), from the pset helpers.
    std::map<std::string, property_map> psets;
    if (facet.propertySet.isSimple()) {
        const auto pset = get_pset(inst, facet.propertySet.simple);
        if (pset) {
            if (const auto* map = pset->get_if<property_map>(); map && !map->empty()) {
                psets[facet.propertySet.simple] = *map;
            }
        }
    } else {
        for (const auto& [pset_name, pset_props] : get_psets(inst)) {
            if (facet.propertySet.matches(Value(pset_name))) psets[pset_name] = pset_props;
        }
    }

    bool is_pass = !psets.empty();
    std::string reason;
    if (!is_pass) {
        if (facet.cardinality == Cardinality::Optional) return pass();
        reason = "The required property set does not exist";
    }

    if (is_pass) {
        ifcopenshell::file* file = inst.file();
        for (const auto& [pset_name, pset_props] : psets) {
            const auto id_it = pset_props.find("id");
            const auto* id = id_it == pset_props.end() ? nullptr : id_it->second.get_if<std::int64_t>();
            const Entity pset_entity = id && file ? file->instance_by_id(static_cast<int>(*id)) : Entity();
            const auto pset_properties = pset_entity ? propertiesOf(pset_entity) : std::optional<std::vector<Entity>>();

            std::map<std::string, Value> props;
            if (facet.baseName.isSimple()) {
                const auto found = pset_props.find(facet.baseName.simple);
                if (found != pset_props.end()) {
                    const Value& prop = found->second;
                    bool unknown_logical = false;
                    if (const auto* text = prop.get_if<std::string>(); text && *text == "UNKNOWN" && pset_properties) {
                        for (const Entity& p : *pset_properties) {
                            if (util::forwardString(p, "Name") != facet.baseName.simple) continue;
                            unknown_logical = util::isA(util::forwardEntity(p, "NominalValue"), "IfcLogical");
                            break;
                        }
                    }
                    const auto* text = prop.get_if<std::string>();
                    if (!unknown_logical && !prop.is_null() && !(text && text->empty())) props[facet.baseName.simple] = prop;
                }
            } else {
                for (const auto& [key, v] : pset_props) {
                    const auto* text = v.get_if<std::string>();
                    if (facet.baseName.matches(Value(key)) && !v.is_null() && !(text && text->empty())) props[key] = v;
                }
            }

            if (props.empty()) {
                if (facet.cardinality == Cardinality::Optional) return pass();
                is_pass = false;
                reason = "The property set does not contain the required property";
                break;
            }

            bool is_property_supported_class = true;
            for (const Entity& prop_entity : pset_properties.value_or(std::vector<Entity>())) {
                const auto prop_name = util::forwardString(prop_entity, "Name");
                if (!prop_name || !props.count(*prop_name)) continue;
                Value& prop_value = props[*prop_name];

                if (util::isA(prop_entity, "IfcPropertySingleValue")) {
                    const std::string data_type = util::className(util::forwardEntity(prop_entity, "NominalValue"));
                    if (dataTypeDiffers(facet, data_type)) {
                        is_pass = false;
                        reason = dataTypeReason(data_type, facet);
                        break;
                    }
                    const Entity unit = util::get_property_unit(prop_entity);
                    if (unit && util::unitName(unit)) {
                        const auto output_prefix =
                            util::unitType(unit) == "MASSUNIT" ? std::optional<std::string>("KILO") : std::nullopt;
                        prop_value = convertValue(prop_value, unit, output_prefix);
                    }
                } else if (util::isA(prop_entity, "IfcPhysicalSimpleQuantity")) {
                    const auto* entity = prop_entity.declaration().as_entity();
                    std::string data_type;
                    if (const auto* named = entity->attribute_by_index(3)->type_of_attribute()->as_named_type()) {
                        data_type = named->declared_type()->name();
                    }
                    if (dataTypeDiffers(facet, data_type)) {
                        is_pass = false;
                        reason = dataTypeReason(data_type, facet);
                        break;
                    }
                    prop_value = convertValue(prop_value, util::get_property_unit(prop_entity), std::nullopt);
                } else if (util::isA(prop_entity, "IfcPropertyEnumeratedValue") ||
                           util::isA(prop_entity, "IfcPropertyListValue")) {
                    const bool enumerated = util::isA(prop_entity, "IfcPropertyEnumeratedValue");
                    const auto values = util::forwardEntities(prop_entity, enumerated ? "EnumerationValues" : "ListValues");
                    if (values.empty()) {
                        is_pass = false;
                        reason = "The property set does not contain the required property";
                        break;
                    }
                    const std::string data_type = util::className(values.front());
                    if (dataTypeDiffers(facet, data_type)) {
                        is_pass = false;
                        reason = dataTypeReason(data_type, facet);
                        break;
                    }
                    if (!enumerated) prop_value = convertList(prop_value, util::get_property_unit(prop_entity));
                } else if (util::isA(prop_entity, "IfcPropertyBoundedValue")) {
                    property_list values;
                    std::string data_type;
                    for (const char* attribute : {"UpperBoundValue", "LowerBoundValue", "SetPointValue"}) {
                        if (const Entity value = util::forwardEntity(prop_entity, attribute)) {
                            data_type = util::className(value);
                            values.push_back(fromAttribute(util::wrappedValue(value)));
                        }
                    }
                    if (data_type.empty()) {  // Python: assert data_type is not None
                        is_pass = false;
                        reason = "The property set does not contain the required property";
                        break;
                    }
                    if (dataTypeDiffers(facet, data_type)) {
                        is_pass = false;
                        reason = dataTypeReason(data_type, facet);
                        break;
                    }
                    prop_value = convertList(values, util::get_property_unit(prop_entity));
                } else if (util::isA(prop_entity, "IfcPropertyTableValue")) {
                    property_list values;
                    const auto [defining_unit, defined_unit] = util::get_property_table_unit(prop_entity);
                    std::string data_type;
                    for (const char* attribute : {"Defining", "Defined"}) {
                        const auto column_values = util::forwardEntities(prop_entity, std::string(attribute) + "Values");
                        if (column_values.empty()) continue;
                        data_type = util::className(column_values.front());
                        if (facet.dataType && lower(data_type) == lower(*facet.dataType)) {
                            const Entity& unit = std::string(attribute) == "Defining" ? defining_unit : defined_unit;
                            for (const Entity& v : column_values) {
                                values.push_back(convertValue(fromAttribute(util::wrappedValue(v)), unit, std::nullopt));
                            }
                        }
                    }
                    if (values.empty()) {
                        is_pass = false;
                        reason = dataTypeReason(data_type.empty() ? "None" : data_type, facet);
                        break;
                    }
                    prop_value = values;
                } else {
                    is_property_supported_class = false;
                }
            }

            if (!is_property_supported_class) {
                is_pass = false;
                reason = "The property set does not contain the required property";
            }
            if (!is_pass) break;

            if (facet.value.truthy()) {
                for (const auto& [key, value] : props) {
                    if (!propertyValueMatches(facet, value)) {
                        is_pass = false;
                        reason = valueReason(value);
                        break;
                    }
                }
            }
            // Python keeps checking the next pset after a failed value test.
        }
    }
    return finish(facet, is_pass, reason, "The property should not have met the requirement");
}

// ---------------------------------------------------------------- Material

Result materialCall(const Facet& facet, const Entity& inst) {
    const Entity material = util::get_material(inst, true);
    bool is_pass = static_cast<bool>(material);
    std::string reason;
    if (!is_pass) {
        if (facet.cardinality == Cardinality::Optional) return pass();
        reason = "The entity has no material";
    }

    if (is_pass && facet.value.truthy()) {
        std::set<std::string> names;  // the Python set, None left out (it never matches)
        const auto add = [&](const Entity& e, const char* attribute) {
            if (const auto text = util::forwardString(e, attribute)) names.insert(*text);
        };
        const auto addItems = [&](const char* list_attribute) {
            for (const Entity& item : util::forwardEntities(material, list_attribute)) {
                add(item, "Name");
                add(item, "Category");
                if (const Entity item_material = util::forwardEntity(item, "Material")) {
                    add(item_material, "Name");
                    add(item_material, "Category");
                }
            }
        };
        if (util::isA(material, "IfcMaterial")) {
            add(material, "Name");
            add(material, "Category");
        } else if (util::isA(material, "IfcMaterialList")) {
            for (const Entity& mat : util::forwardEntities(material, "Materials")) {
                add(mat, "Name");
                add(mat, "Category");
            }
        } else if (util::isA(material, "IfcMaterialLayerSet")) {
            add(material, "LayerSetName");
            addItems("MaterialLayers");
        } else if (util::isA(material, "IfcMaterialProfileSet")) {
            add(material, "Name");
            addItems("MaterialProfiles");
        } else if (util::isA(material, "IfcMaterialConstituentSet")) {
            add(material, "Name");
            addItems("MaterialConstituents");
        } else {
            // Python: assert False (a layer, profile or constituent assigned
            // directly). The port reads its name and category.
            add(material, "Name");
            add(material, "Category");
        }
        is_pass = std::any_of(names.begin(), names.end(), [&](const std::string& n) { return facet.value.matches(Value(n)); });
        if (!is_pass) {
            std::vector<Value> values(names.begin(), names.end());
            reason = "The material names and categories of \"" + reprSet(values) + "\" does not match the requirement";
        }
    }
    return finish(facet, is_pass, reason, "The material should not have met the requirement");
}

// --------------------------------------------------------------- to_string

struct Templates {
    std::vector<std::string> applicability;
    std::vector<std::string> requirement;
    std::vector<std::string> prohibited;
};

const Templates& templatesOf(FacetType type) {
    static const std::map<FacetType, Templates> templates = {
        {FacetType::Entity,
         {{"All {name} data of type {predefinedType}", "All {name} data"},
          {"Shall be {name} data of type {predefinedType}", "Shall be {name} data"},
          {"Shall not be {name} data of type {predefinedType}", "Shall not be {name} data"}}},
        {FacetType::Attribute,
         {{"Data where the {name} is {value}", "Data where the {name} is provided"},
          {"The {name} shall be {value}", "The {name} shall be provided"},
          {"The {name} shall not be {value}", "The {name} shall not be provided"}}},
        {FacetType::Classification,
         {{"Data having a {system} reference of {value}", "Data classified using {system}", "Data classified as {value}"},
          {"Shall have a {system} reference of {value}", "Shall be classified using {system}",
           "Shall be classified as {value}"},
          {"Shall not have a {system} reference of {value}", "Shall not be classified using {system}",
           "Shall not be classified as {value}"}}},
        {FacetType::PartOf,
         {{"An element with an {relation} relationship with an {name}", "An element with an {relation} relationship",
           "An element that is part of an {name} of predefined type {predefinedType}",
           "An element that is part of an {name}"},
          {"An element must have an {relation} relationship with an {name} of predefined type {predefinedType}",
           "An element must have an {relation} relationship with an {name}",
           "An element must have an {relation} relationship",
           "An element must be part of an {name} of predefined type {predefinedType}",
           "An element must be part of an {name}"},
          {"An element must not have an {relation} relationship with an {name}",
           "An element must not have an {relation} relationship",
           "An element must not be part of an {name} of predefined type {predefinedType}",
           "An element must not be part of an {name}"}}},
        {FacetType::Property,
         {{"Elements with {baseName} data of {value} in the dataset {propertySet}",
           "Elements with {baseName} data in the dataset {propertySet}"},
          {"{baseName} data shall be {value} and in the dataset {propertySet}",
           "{baseName} data shall be provided in the dataset {propertySet}"},
          {"{baseName} data shall not be {value} and in the dataset {propertySet}",
           "{baseName} data shall not be provided in the dataset {propertySet}"}}},
        {FacetType::Material,
         {{"All data with a {value} material", "All data with a material"},
          {"Shall have a material of {value}", "Shall have a material"},
          {"Shall not have a material of {value}", "Shall not have a material"}}},
    };
    return templates.at(type);
}

std::optional<std::string> paramText(const Param& param) {
    return param.isNone() ? std::nullopt : std::optional<std::string>(param.toString());
}

// self.parameters, in order, with str(value) (nullopt for None).
std::vector<std::pair<std::string, std::optional<std::string>>> parameters(const Facet& f) {
    const auto cardinality = std::optional<std::string>(cardinalityName(f.cardinality));
    switch (f.type) {
    case FacetType::Entity:
        return {{"name", paramText(f.name)}, {"predefinedType", paramText(f.predefinedType)}, {"instructions", f.instructions}};
    case FacetType::Attribute:
        return {{"name", paramText(f.name)}, {"value", paramText(f.value)}, {"cardinality", cardinality},
                {"instructions", f.instructions}};
    case FacetType::Classification:
        return {{"value", paramText(f.value)}, {"system", paramText(f.system)}, {"uri", f.uri},
                {"cardinality", cardinality}, {"instructions", f.instructions}};
    case FacetType::PartOf:
        return {{"name", paramText(f.name)}, {"predefinedType", paramText(f.predefinedType)}, {"relation", f.relation},
                {"cardinality", cardinality}, {"instructions", f.instructions}};
    case FacetType::Property:
        return {{"propertySet", paramText(f.propertySet)}, {"baseName", paramText(f.baseName)},
                {"value", paramText(f.value)}, {"dataType", f.dataType}, {"uri", f.uri},
                {"cardinality", cardinality}, {"instructions", f.instructions}};
    case FacetType::Material:
        return {{"value", paramText(f.value)}, {"uri", f.uri}, {"cardinality", cardinality},
                {"instructions", f.instructions}};
    }
    return {};
}

} // namespace

std::string facetTypeName(FacetType type) {
    switch (type) {
    case FacetType::Entity: return "entity";
    case FacetType::Attribute: return "attribute";
    case FacetType::Classification: return "classification";
    case FacetType::PartOf: return "partOf";
    case FacetType::Property: return "property";
    case FacetType::Material: return "material";
    }
    return {};
}

std::optional<FacetType> facetTypeFromName(const std::string& name) {
    for (FacetType type : {FacetType::Entity, FacetType::Attribute, FacetType::Classification, FacetType::PartOf,
                           FacetType::Property, FacetType::Material}) {
        if (facetTypeName(type) == name) return type;
    }
    return std::nullopt;
}

std::string cardinalityName(Cardinality cardinality) {
    switch (cardinality) {
    case Cardinality::Required: return "required";
    case Cardinality::Optional: return "optional";
    case Cardinality::Prohibited: return "prohibited";
    }
    return "required";
}

std::optional<Cardinality> cardinalityFromName(const std::string& name) {
    if (name == "required") return Cardinality::Required;
    if (name == "optional") return Cardinality::Optional;
    if (name == "prohibited") return Cardinality::Prohibited;
    return std::nullopt;
}

Facet Facet::make(FacetType type) {
    Facet facet;
    facet.type = type;
    switch (type) {
    case FacetType::Entity: facet.name = Param::text("IFCWALL"); break;
    case FacetType::Attribute: facet.name = Param::text("Name"); break;
    case FacetType::PartOf: facet.name = Param::text("IFCWALL"); break;
    case FacetType::Property:
        facet.propertySet = Param::text("Property_Set");
        facet.baseName = Param::text("PropertyName");
        break;
    case FacetType::Classification:
    case FacetType::Material: break;
    }
    return facet;
}

std::vector<Entity> Facet::filter(ifcopenshell::file& file, const std::vector<Entity>* elements) const {
    if (elements) {
        // Facet.filter: [e for e in elements if self(e)]
        std::vector<Entity> results;
        for (const Entity& element : *elements) {
            if ((*this)(element)) results.push_back(element);
        }
        return results;
    }
    switch (type) {
    case FacetType::Entity: return entityFilter(*this, file);
    case FacetType::Attribute: return attributeFilter(*this, file);
    case FacetType::Property: return propertyFilter(file);
    // PartOf: list(ifc_file) — only object definitions can pass it.
    case FacetType::PartOf:
    case FacetType::Classification:
    case FacetType::Material: return byType(file, "IfcObjectDefinition");
    }
    return {};
}

Result Facet::operator()(const Entity& inst) const {
    if (!inst) return fail({});
    switch (type) {
    case FacetType::Entity: return entityCall(*this, inst);
    case FacetType::Attribute: return attributeCall(*this, inst);
    case FacetType::Classification: return classificationCall(*this, inst);
    case FacetType::PartOf: return partOfCall(*this, inst);
    case FacetType::Property: return propertyCall(*this, inst);
    case FacetType::Material: return materialCall(*this, inst);
    }
    return fail({});
}

std::string Facet::toString(Clause clause, bool specification_prohibited, const Facet* requirement) const {
    const Templates& all = templatesOf(type);
    std::vector<std::string> templates;
    if (clause == Clause::Applicability) {
        templates = specification_prohibited ? all.prohibited : all.applicability;
    } else {
        if (specification_prohibited) return "The requirement is not applicable";
        if (!requirement || requirement->type == FacetType::Entity || requirement->cardinality == Cardinality::Required) {
            templates = all.requirement;
        } else if (requirement->cardinality == Cardinality::Prohibited) {
            templates = all.prohibited;
        } else {
            templates = all.requirement;
            for (std::string& t : templates) {
                replaceAll(t, "shall", "may");
                replaceAll(t, "Shall", "May");
                replaceAll(t, "must", "may");
            }
        }
    }

    const auto params = parameters(*this);
    for (std::string text : templates) {
        const auto total_variables = static_cast<std::size_t>(std::count(text.begin(), text.end(), '{'));
        std::size_t total_replacements = 0;
        for (const auto& [key, value] : params) {
            const std::string key_variable = "{" + key + "}";
            if (value && text.find(key_variable) != std::string::npos) {
                replaceAll(text, key_variable, *value);
                ++total_replacements;
            }
            if (total_replacements == total_variables) return text;
        }
    }
    return "This facet cannot be interpreted";
}

} // namespace bonsaiviewer::ports::ifctester
