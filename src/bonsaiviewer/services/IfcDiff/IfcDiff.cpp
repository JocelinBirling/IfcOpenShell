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

// PORT of src/ifcdiff/ifcdiff.py — see IfcDiff.h.

#include "IfcDiff.h"

#include "../../ports/ifcopenshell_util/Attr.h"
#include "../../ports/ifcopenshell_util/Classification.h"
#include "../../ports/ifcopenshell_util/Element.h"
#include "../../ports/ifctester/PyValue.h"
#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"

#include "placement.h" // helpers: get_local_placement
#include "pset.h"      // helpers: get_psets

#include <QJsonArray>
#include <QString>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>

namespace bonsaiviewer::services::ifcdiff {

namespace util = bonsaiviewer::ports::ifcopenshell_util;
namespace it = bonsaiviewer::ports::ifctester;

namespace {

using Entity = express::base;

// The walk of a representation item stops recording paths past this many
// differences (the item is reported changed either way).
constexpr std::size_t kMaxItemChanges = 50;
constexpr int kMaxDepth = 64;
// diff_element_basic_geometry: the rotation part of the placement.
constexpr double kRotationTolerance = 1e-2;

std::string guidOf(const Entity& element) {
    return util::forwardString(element, "GlobalId").value_or(std::string());
}

std::string nameOf(const Entity& element) {
    return util::forwardString(element, "Name").value_or(std::string());
}

bool isAggregate(ifcopenshell::argument_type type) {
    return type >= ifcopenshell::Argument_EMPTY_AGGREGATE && type <= ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE;
}

std::string formatDouble(double value) {
    return it::pyFloatRepr(value);
}

std::string formatDoubles(const std::vector<double>& values) {
    std::string text = "(";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) text += ", ";
        text += formatDouble(values[i]);
    }
    return text + ")";
}

// A scalar attribute value, with the numeric / string type changes ignored
// (DeepDiff ignore_numeric_type_changes / ignore_string_type_changes).
struct Scalar {
    enum class Kind { Null, Number, Text, Bool, Other } kind = Kind::Null;
    double number = 0.0;
    std::string text;
};

Scalar scalarOf(const ifcopenshell::attribute_value& value) {
    Scalar scalar;
    if (value.isNull()) return scalar;
    switch (value.type()) {
    case ifcopenshell::Argument_INT:
        scalar.kind = Scalar::Kind::Number;
        scalar.number = static_cast<double>(static_cast<int64_t>(value));
        scalar.text = std::to_string(static_cast<int64_t>(value));
        break;
    case ifcopenshell::Argument_DOUBLE:
        scalar.kind = Scalar::Kind::Number;
        scalar.number = static_cast<double>(value);
        scalar.text = formatDouble(scalar.number);
        break;
    case ifcopenshell::Argument_STRING:
        scalar.kind = Scalar::Kind::Text;
        scalar.text = static_cast<std::string>(value);
        break;
    case ifcopenshell::Argument_ENUMERATION: {
        const ifcopenshell::enumeration_reference enumeration = value;
        scalar.kind = Scalar::Kind::Text;
        scalar.text = enumeration.value() ? enumeration.value() : "";
        break;
    }
    case ifcopenshell::Argument_BOOL:
        scalar.kind = Scalar::Kind::Bool;
        scalar.text = static_cast<bool>(value) ? "True" : "False";
        break;
    case ifcopenshell::Argument_LOGICAL: {
        const boost::logic::tribool logical = value;
        scalar.kind = Scalar::Kind::Bool;
        scalar.text = boost::logic::indeterminate(logical) ? "UNKNOWN" : (static_cast<bool>(logical) ? "True" : "False");
        break;
    }
    case ifcopenshell::Argument_DERIVED:
        scalar.kind = Scalar::Kind::Other;
        scalar.text = "*";
        break;
    default:
        scalar.kind = Scalar::Kind::Other;
        break;
    }
    return scalar;
}

bool scalarsEqual(const Scalar& a, const Scalar& b, double epsilon) {
    if (a.kind != b.kind) return false;
    if (a.kind == Scalar::Kind::Number) return std::abs(a.number - b.number) <= epsilon;
    return a.text == b.text;
}

bool doublesEqual(const std::vector<double>& a, const std::vector<double>& b, double epsilon) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::abs(a[i] - b[i]) > epsilon) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// The element universe (IfcDiff.diff): IfcElement + spatial elements, minus
// the feature elements, by GlobalId.

std::map<std::string, Entity> elementsByGuid(ifcopenshell::file& file) {
    std::map<std::string, Entity> elements;
    const std::string schema = file.schema() ? file.schema()->name() : std::string();
    const char* spatial = schema == "IFC2X3" ? "IfcSpatialStructureElement" : "IfcSpatialElement";
    for (const char* ifc_class : {"IfcElement", spatial}) {
        std::vector<Entity> instances;
        try {
            instances = file.instances_by_type(ifc_class);
        } catch (...) {
            continue;
        }
        for (const Entity& element : instances) {
            if (util::isA(element, "IfcFeatureElement")) continue;
            const std::string guid = guidOf(element);
            if (!guid.empty()) elements.emplace(guid, element);
        }
    }
    return elements;
}

// get_precision: the Precision of the first Model context of the new file.
double precisionOf(ifcopenshell::file& file) {
    std::vector<Entity> contexts;
    try {
        contexts = file.instances_by_type("IfcGeometricRepresentationContext");
    } catch (...) {
        return 1e-4;
    }
    for (const Entity& context : contexts) {
        if (util::forwardString(context, "ContextType").value_or("") != "Model") continue;
        const auto precision = util::forward(context, "Precision");
        if (precision && precision->type() == ifcopenshell::Argument_DOUBLE) {
            const double value = static_cast<double>(*precision);
            if (value > 0.0) return value;
        }
        return 1e-4;
    }
    return 1e-4;
}

// ---------------------------------------------------------------------------
// attributes (diff_element): the attributes that are not entity instances
// nor aggregates.

std::vector<AttributeChange> diffAttributes(const Entity& old_element, const Entity& new_element, double epsilon) {
    std::vector<AttributeChange> changes;
    const auto* old_entity = old_element.declaration().as_entity();
    const auto* new_entity = new_element.declaration().as_entity();
    if (old_entity == nullptr || new_entity == nullptr) return changes;
    const auto& new_attributes = new_entity->all_attributes();
    for (std::size_t index = 0; index < new_attributes.size(); ++index) {
        const std::string& name = new_attributes[index]->name();
        const auto old_index = old_entity->attribute_index(name);
        const auto new_value = new_element.get_attribute_value(index);
        const bool new_skipped = new_value.type() == ifcopenshell::Argument_ENTITY_INSTANCE || isAggregate(new_value.type());
        if (old_index < 0) {
            if (!new_skipped && !new_value.isNull()) changes.push_back({name, "", scalarOf(new_value).text});
            continue;
        }
        const auto old_value = old_element.get_attribute_value(static_cast<std::size_t>(old_index));
        const bool old_skipped = old_value.type() == ifcopenshell::Argument_ENTITY_INSTANCE || isAggregate(old_value.type());
        if (old_skipped || new_skipped) continue;
        const Scalar a = scalarOf(old_value);
        const Scalar b = scalarOf(new_value);
        if (!scalarsEqual(a, b, epsilon)) changes.push_back({name, a.text, b.text});
    }
    return changes;
}

// ---------------------------------------------------------------------------
// property (diff_element_relationships, "property"): get_psets of both
// elements, compared property by property.

bool isEmptyValue(const property_value& value) {
    if (value.is_null()) return true;
    if (const auto* text = value.get_if<std::string>()) return text->empty();
    if (const auto* list = value.get_if<property_list>()) return list->empty();
    return false;
}

std::optional<double> numberOf(const property_value& value) {
    if (const auto* i = value.get_if<std::int64_t>()) return static_cast<double>(*i);
    if (const auto* d = value.get_if<double>()) return *d;
    return std::nullopt;
}

bool valuesEqual(const property_value& a, const property_value& b, double epsilon, int depth = 0) {
    if (depth > kMaxDepth) return true;
    const auto na = numberOf(a);
    const auto nb = numberOf(b);
    if (na || nb) return na && nb && std::abs(*na - *nb) <= epsilon;
    if (a.value.index() != b.value.index()) return false;
    if (a.is_null()) return true;
    if (const auto* x = a.get_if<bool>()) return *x == *b.get_if<bool>();
    if (const auto* x = a.get_if<std::string>()) return *x == *b.get_if<std::string>();
    if (const auto* x = a.get_if<express::base>()) {
        const auto* y = b.get_if<express::base>();
        return util::className(*x) == util::className(*y);
    }
    if (const auto* x = a.get_if<property_list>()) {
        const auto* y = b.get_if<property_list>();
        if (x->size() != y->size()) return false;
        for (std::size_t i = 0; i < x->size(); ++i) {
            if (!valuesEqual((*x)[i], (*y)[i], epsilon, depth + 1)) return false;
        }
        return true;
    }
    if (const auto* x = a.get_if<property_map>()) {
        const auto* y = b.get_if<property_map>();
        // exclude_regex_paths=[r".*id$"]: the definition ids differ between files.
        auto key_count = [](const property_map& map) {
            return std::count_if(map.begin(), map.end(), [](const auto& entry) { return entry.first != "id"; });
        };
        if (key_count(*x) != key_count(*y)) return false;
        for (const auto& [key, value] : *x) {
            if (key == "id") continue;
            const auto other = y->find(key);
            if (other == y->end() || !valuesEqual(value, other->second, epsilon, depth + 1)) return false;
        }
        return true;
    }
    return true;
}

std::string valueText(const property_value& value) {
    if (value.is_null()) return {};
    return it::pyStr(value);
}

using PropertyFilter = std::function<bool(const std::string& pset, const std::string& property)>;

std::vector<PropertyChange> diffProperties(const element_properties& old_psets, const element_properties& new_psets,
                                           double epsilon, const PropertyFilter& keep) {
    std::vector<PropertyChange> changes;
    std::set<std::string> pset_names;
    for (const auto& [name, properties] : old_psets) pset_names.insert(name);
    for (const auto& [name, properties] : new_psets) pset_names.insert(name);
    static const property_map no_properties;
    for (const std::string& pset : pset_names) {
        const auto old_found = old_psets.find(pset);
        const auto new_found = new_psets.find(pset);
        const property_map& old_properties = old_found == old_psets.end() ? no_properties : old_found->second;
        const property_map& new_properties = new_found == new_psets.end() ? no_properties : new_found->second;
        std::set<std::string> names;
        for (const auto& [name, value] : old_properties) names.insert(name);
        for (const auto& [name, value] : new_properties) names.insert(name);
        for (const std::string& name : names) {
            if (name == "id") continue;
            if (keep && !keep(pset, name)) continue;
            const auto old_value = old_properties.find(name);
            const auto new_value = new_properties.find(name);
            PropertyChange change;
            change.pset = pset;
            change.property = name;
            if (old_value == old_properties.end()) {
                change.kind = PropertyChangeKind::PropertyAdded;
                change.new_value = valueText(new_value->second);
            } else if (new_value == new_properties.end()) {
                change.kind = PropertyChangeKind::PropertyRemoved;
                change.old_value = valueText(old_value->second);
            } else {
                if (valuesEqual(old_value->second, new_value->second, epsilon)) continue;
                change.old_value = valueText(old_value->second);
                change.new_value = valueText(new_value->second);
                const bool old_empty = isEmptyValue(old_value->second);
                const bool new_empty = isEmptyValue(new_value->second);
                if (old_empty && !new_empty) {
                    change.kind = PropertyChangeKind::ValueAdded;
                } else if (!old_empty && new_empty) {
                    change.kind = PropertyChangeKind::ValueRemoved;
                } else {
                    change.kind = PropertyChangeKind::ValueChanged;
                    const auto a = numberOf(old_value->second);
                    const auto b = numberOf(new_value->second);
                    if (a && b) change.direction = *b > *a ? 1 : -1;
                }
            }
            changes.push_back(std::move(change));
        }
    }
    return changes;
}

// ---------------------------------------------------------------------------
// type / container / aggregate / classification

bool sameByGuid(const Entity& a, const Entity& b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return guidOf(a) == guidOf(b);
}

std::vector<std::string> classificationReferences(const Entity& element) {
    const char* attribute = util::schemaName(element) == "IFC2X3" ? "ItemReference" : "Identification";
    std::vector<std::string> references;
    for (const Entity& reference : util::get_references(element)) {
        references.push_back(util::forwardString(reference, attribute).value_or(std::string()));
    }
    std::sort(references.begin(), references.end());
    return references;
}

// ---------------------------------------------------------------------------
// geometry: placement, openings, projections (diff_element_basic_geometry)
// and the Body representation items (diff_representation_item).

std::vector<std::string> relatedGuids(const Entity& element, const char* inverse, const char* related) {
    std::vector<std::string> guids;
    for (const Entity& relationship : util::inverse(element, inverse)) {
        const std::string guid = guidOf(util::forwardEntity(relationship, related));
        if (!guid.empty()) guids.push_back(guid);
    }
    std::sort(guids.begin(), guids.end());
    return guids;
}

void setDifference(const std::vector<std::string>& from, const std::vector<std::string>& minus,
                   std::vector<std::string>& out) {
    std::set_difference(from.begin(), from.end(), minus.begin(), minus.end(), std::back_inserter(out));
}

// get_representation_id: the Body IfcShapeRepresentation of the element.
Entity bodyRepresentation(const Entity& element) {
    const Entity shape = util::forwardEntity(element, "Representation");
    for (const Entity& representation : util::forwardEntities(shape, "Representations")) {
        if (!util::isA(representation, "IfcShapeRepresentation")) continue;
        if (util::forwardString(representation, "RepresentationIdentifier").value_or("") == "Body") return representation;
    }
    return {};
}

// Recursive comparison of two representation items over their EXPRESS
// attributes; numbers within the precision, ids ignored (".*id']$").
class ItemComparer {
public:
    ItemComparer(double epsilon, bool shallow) : epsilon_(epsilon), shallow_(shallow) {}

    // True when equal. The differences are added to `changes` (up to
    // kMaxItemChanges, the first one only when shallow).
    bool compare(const Entity& a, const Entity& b, std::vector<ItemAttributeChange>* changes) {
        return compareEntities(a, b, std::string(), changes, 0);
    }

private:
    bool recording(const std::vector<ItemAttributeChange>* changes) const {
        return changes != nullptr && changes->size() < (shallow_ ? 1u : kMaxItemChanges);
    }

    void record(std::vector<ItemAttributeChange>* changes, const std::string& path, std::string old_value,
                std::string new_value) {
        if (recording(changes)) changes->push_back({path.empty() ? std::string("(item)") : path, std::move(old_value), std::move(new_value)});
    }

    static std::string join(const std::string& path, const std::string& name) {
        return path.empty() ? name : path + "." + name;
    }

    bool compareEntities(const Entity& a, const Entity& b, const std::string& path,
                         std::vector<ItemAttributeChange>* changes, int depth) {
        if (!a || !b) {
            if (!a && !b) return true;
            record(changes, path, a ? util::className(a) : "None", b ? util::className(b) : "None");
            return false;
        }
        const std::string class_a = util::className(a);
        const std::string class_b = util::className(b);
        if (class_a != class_b) {
            record(changes, path, class_a, class_b);
            return false;
        }
        if (depth > kMaxDepth) return true;
        const std::pair<unsigned, unsigned> key{a.id(), b.id()};
        const auto known = memo_.find(key);
        // A known result is enough when nothing more is to be recorded.
        if (known != memo_.end() && (known->second || !recording(changes))) return known->second;

        bool equal = true;
        const auto* entity = a.declaration().as_entity();
        if (entity != nullptr) {
            const auto& attributes = entity->all_attributes();
            for (std::size_t index = 0; index < attributes.size(); ++index) {
                const std::string& name = attributes[index]->name();
                if (!compareValues(a.get_attribute_value(index), b.get_attribute_value(index), join(path, name), changes, depth)) {
                    equal = false;
                    if (shallow_ || !recording(changes)) break;
                }
            }
        }
        memo_[key] = equal;
        return equal;
    }

    bool compareValues(const ifcopenshell::attribute_value& a, const ifcopenshell::attribute_value& b,
                       const std::string& path, std::vector<ItemAttributeChange>* changes, int depth) {
        const auto type_a = a.isNull() ? ifcopenshell::Argument_NULL : a.type();
        const auto type_b = b.isNull() ? ifcopenshell::Argument_NULL : b.type();
        const bool entities = type_a == ifcopenshell::Argument_ENTITY_INSTANCE || type_b == ifcopenshell::Argument_ENTITY_INSTANCE;
        if (entities) {
            const Entity ea = type_a == ifcopenshell::Argument_ENTITY_INSTANCE ? static_cast<Entity>(a) : Entity();
            const Entity eb = type_b == ifcopenshell::Argument_ENTITY_INSTANCE ? static_cast<Entity>(b) : Entity();
            return compareEntities(ea, eb, path, changes, depth + 1);
        }
        if (type_a == ifcopenshell::Argument_AGGREGATE_OF_ENTITY_INSTANCE ||
            type_b == ifcopenshell::Argument_AGGREGATE_OF_ENTITY_INSTANCE) {
            if (type_a != type_b) {
                record(changes, path, "", "");
                return false;
            }
            const std::vector<Entity> la = a;
            const std::vector<Entity> lb = b;
            return compareLists(la, lb, path, changes, depth);
        }
        if (type_a == ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE ||
            type_b == ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE) {
            if (type_a != type_b) {
                record(changes, path, "", "");
                return false;
            }
            const std::vector<std::vector<Entity>> la = a;
            const std::vector<std::vector<Entity>> lb = b;
            if (la.size() != lb.size()) {
                record(changes, path, std::to_string(la.size()) + " lists", std::to_string(lb.size()) + " lists");
                return false;
            }
            bool equal = true;
            for (std::size_t i = 0; i < la.size(); ++i) {
                if (!compareLists(la[i], lb[i], path + "[" + std::to_string(i) + "]", changes, depth)) {
                    equal = false;
                    if (shallow_ || !recording(changes)) break;
                }
            }
            return equal;
        }
        if (type_a == ifcopenshell::Argument_AGGREGATE_OF_DOUBLE || type_b == ifcopenshell::Argument_AGGREGATE_OF_DOUBLE ||
            type_a == ifcopenshell::Argument_AGGREGATE_OF_INT || type_b == ifcopenshell::Argument_AGGREGATE_OF_INT) {
            const auto numbers = [](const ifcopenshell::attribute_value& value, ifcopenshell::argument_type type) {
                std::vector<double> result;
                if (type == ifcopenshell::Argument_AGGREGATE_OF_DOUBLE) {
                    result = static_cast<std::vector<double>>(value);
                } else if (type == ifcopenshell::Argument_AGGREGATE_OF_INT) {
                    for (int64_t i : static_cast<std::vector<int64_t>>(value)) result.push_back(static_cast<double>(i));
                }
                return result;
            };
            const auto da = numbers(a, type_a);
            const auto db = numbers(b, type_b);
            if (doublesEqual(da, db, epsilon_)) return true;
            record(changes, path, formatDoubles(da), formatDoubles(db));
            return false;
        }
        if (type_a == ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_DOUBLE ||
            type_b == ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_DOUBLE ||
            type_a == ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_INT ||
            type_b == ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_INT) {
            const auto rows = [](const ifcopenshell::attribute_value& value, ifcopenshell::argument_type type) {
                std::vector<std::vector<double>> result;
                if (type == ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_DOUBLE) {
                    result = static_cast<std::vector<std::vector<double>>>(value);
                } else if (type == ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_INT) {
                    for (const auto& row : static_cast<std::vector<std::vector<int64_t>>>(value)) {
                        result.emplace_back(row.begin(), row.end());
                    }
                }
                return result;
            };
            const auto ra = rows(a, type_a);
            const auto rb = rows(b, type_b);
            if (ra.size() != rb.size()) {
                record(changes, path, std::to_string(ra.size()) + " rows", std::to_string(rb.size()) + " rows");
                return false;
            }
            for (std::size_t i = 0; i < ra.size(); ++i) {
                if (!doublesEqual(ra[i], rb[i], epsilon_)) {
                    record(changes, path + "[" + std::to_string(i) + "]", formatDoubles(ra[i]), formatDoubles(rb[i]));
                    return false;
                }
            }
            return true;
        }
        if (isAggregate(type_a) || isAggregate(type_b)) {
            // Strings, binaries, empty aggregates: compared as their size.
            if (type_a == type_b && a.size() == b.size()) return true;
            record(changes, path, std::to_string(type_a == ifcopenshell::Argument_NULL ? 0 : a.size()) + " values",
                   std::to_string(type_b == ifcopenshell::Argument_NULL ? 0 : b.size()) + " values");
            return false;
        }
        const Scalar sa = scalarOf(a);
        const Scalar sb = scalarOf(b);
        if (scalarsEqual(sa, sb, epsilon_)) return true;
        record(changes, path, sa.kind == Scalar::Kind::Null ? "None" : sa.text, sb.kind == Scalar::Kind::Null ? "None" : sb.text);
        return false;
    }

    bool compareLists(const std::vector<Entity>& la, const std::vector<Entity>& lb, const std::string& path,
                      std::vector<ItemAttributeChange>* changes, int depth) {
        if (la.size() != lb.size()) {
            record(changes, path, std::to_string(la.size()) + " items", std::to_string(lb.size()) + " items");
            return false;
        }
        bool equal = true;
        for (std::size_t i = 0; i < la.size(); ++i) {
            if (!compareEntities(la[i], lb[i], path + "[" + std::to_string(i) + "]", changes, depth + 1)) {
                equal = false;
                if (shallow_ || !recording(changes)) break;
            }
        }
        return equal;
    }

    double epsilon_;
    bool shallow_;
    std::map<std::pair<unsigned, unsigned>, bool> memo_;
};

bool translationsClose(const Eigen::Matrix4d& a, const Eigen::Matrix4d& b, double tolerance) {
    for (int row = 0; row < 3; ++row) {
        if (std::abs(a(row, 3) - b(row, 3)) > tolerance) return false;
    }
    return true;
}

bool rotationsClose(const Eigen::Matrix4d& a, const Eigen::Matrix4d& b) {
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            if (std::abs(a(row, column) - b(row, column)) > kRotationTolerance) return false;
        }
    }
    return true;
}

// Fills `detail`; true when the geometry differs.
bool diffGeometry(const Entity& old_element, const Entity& new_element, double precision, bool shallow,
                  ItemComparer& comparer, GeometryDetail& detail) {
    bool changed = false;
    const Entity old_placement = util::forwardEntity(old_element, "ObjectPlacement");
    const Entity new_placement = util::forwardEntity(new_element, "ObjectPlacement");
    if (old_placement || new_placement) {
        const Eigen::Matrix4d a = old_placement ? get_local_placement(old_placement) : Eigen::Matrix4d::Identity();
        const Eigen::Matrix4d b = new_placement ? get_local_placement(new_placement) : Eigen::Matrix4d::Identity();
        for (int i = 0; i < 3; ++i) {
            detail.translation_old[static_cast<std::size_t>(i)] = a(i, 3);
            detail.translation_new[static_cast<std::size_t>(i)] = b(i, 3);
        }
        detail.moved = !translationsClose(a, b, precision);
        detail.rotation_changed = !rotationsClose(a, b);
        detail.placement_changed = detail.moved || detail.rotation_changed;
        if (detail.placement_changed) {
            changed = true;
            if (shallow) return true;
        }
    }

    const auto old_openings = relatedGuids(old_element, "HasOpenings", "RelatedOpeningElement");
    const auto new_openings = relatedGuids(new_element, "HasOpenings", "RelatedOpeningElement");
    setDifference(new_openings, old_openings, detail.openings_added);
    setDifference(old_openings, new_openings, detail.openings_removed);
    const auto old_projections = relatedGuids(old_element, "HasProjections", "RelatedFeatureElement");
    const auto new_projections = relatedGuids(new_element, "HasProjections", "RelatedFeatureElement");
    setDifference(new_projections, old_projections, detail.projections_added);
    setDifference(old_projections, new_projections, detail.projections_removed);
    if (!detail.openings_added.empty() || !detail.openings_removed.empty() || !detail.projections_added.empty() ||
        !detail.projections_removed.empty()) {
        changed = true;
        if (shallow) return true;
    }

    const Entity old_body = bodyRepresentation(old_element);
    const Entity new_body = bodyRepresentation(new_element);
    detail.representation_id_old = old_body ? static_cast<int>(old_body.id()) : 0;
    detail.representation_id_new = new_body ? static_cast<int>(new_body.id()) : 0;
    if (!old_body && !new_body) return changed;
    const auto old_items = util::forwardEntities(old_body, "Items");
    const auto new_items = util::forwardEntities(new_body, "Items");
    detail.items_count_old = static_cast<int>(old_items.size());
    detail.items_count_new = static_cast<int>(new_items.size());
    if (old_items.size() != new_items.size()) {
        detail.representation_changed = true;
        return true;
    }
    for (std::size_t i = 0; i < old_items.size(); ++i) {
        ItemChange item;
        item.index = static_cast<int>(i);
        item.type_old = util::className(old_items[i]);
        item.type_new = util::className(new_items[i]);
        if (comparer.compare(old_items[i], new_items[i], &item.attributes)) continue;
        detail.representation_changed = true;
        detail.items_changed.push_back(std::move(item));
        if (shallow) break; // DiffTerminator
    }
    return changed || detail.representation_changed;
}

// ---------------------------------------------------------------------------
// IDS shortlist: the elements each specification applies to, and the
// property requirements of the specification.

struct Shortlist {
    std::vector<std::vector<it::Facet>> property_facets;       // per specification
    std::map<std::string, std::vector<std::size_t>> by_guid;   // GlobalId -> specifications
};

Shortlist buildShortlist(const it::Ids& ids, ifcopenshell::file& old_file, ifcopenshell::file& new_file) {
    Shortlist shortlist;
    for (std::size_t s = 0; s < ids.specifications.size(); ++s) {
        std::vector<it::Facet> facets;
        for (const it::Facet& facet : ids.specifications[s].requirements) {
            if (facet.type == it::FacetType::Property) facets.push_back(facet);
        }
        shortlist.property_facets.push_back(facets);
        if (facets.empty()) continue;
        for (ifcopenshell::file* file : {&old_file, &new_file}) {
            it::Specification spec = ids.specifications[s];
            try {
                // As Ids.validate: only the specifications of the file's IFC version.
                if (!spec.check_ifc_version(*file)) continue;
                spec.validate(*file, true);
            } catch (...) {
                continue;
            }
            for (const Entity& element : spec.applicable_entities) {
                auto& specs = shortlist.by_guid[guidOf(element)];
                if (std::find(specs.begin(), specs.end(), s) == specs.end()) specs.push_back(s);
            }
        }
    }
    return shortlist;
}

std::string pathOf(const PropertyChange& change) {
    return "root['" + change.pset + "']['" + change.property + "']";
}

QJsonArray toJsonArray(const std::vector<std::string>& values) {
    QJsonArray array;
    for (const std::string& value : values) array.append(QString::fromStdString(value));
    return array;
}

QJsonArray toJsonArray(const std::array<double, 3>& values) {
    return QJsonArray{values[0], values[1], values[2]};
}

QJsonValue jsonValue(const std::string& text) {
    if (text.empty()) return QJsonValue::Null;
    return QString::fromStdString(text);
}

// The DeepDiff-like dictionary of properties_changed.
QJsonObject propertiesJson(const std::vector<PropertyChange>& changes) {
    QJsonObject values_changed;
    QJsonObject type_changes;
    QJsonArray added;
    QJsonArray removed;
    for (const PropertyChange& change : changes) {
        const QString path = QString::fromStdString(pathOf(change));
        switch (change.kind) {
        case PropertyChangeKind::PropertyAdded:
            added.append(path);
            break;
        case PropertyChangeKind::PropertyRemoved:
            removed.append(path);
            break;
        case PropertyChangeKind::ValueAdded:
        case PropertyChangeKind::ValueRemoved:
            type_changes[path] = QJsonObject{{"old_value", jsonValue(change.old_value)},
                                             {"new_value", jsonValue(change.new_value)}};
            break;
        case PropertyChangeKind::ValueChanged:
            values_changed[path] = QJsonObject{{"old_value", jsonValue(change.old_value)},
                                               {"new_value", jsonValue(change.new_value)}};
            break;
        }
    }
    QJsonObject json;
    if (!values_changed.isEmpty()) json["values_changed"] = values_changed;
    if (!type_changes.isEmpty()) json["type_changes"] = type_changes;
    if (!added.isEmpty()) json["dictionary_item_added"] = added;
    if (!removed.isEmpty()) json["dictionary_item_removed"] = removed;
    return json;
}

QJsonObject geometryJson(const GeometryDetail& detail) {
    QJsonObject json;
    if (detail.placement_changed) {
        std::array<double, 3> delta{};
        for (std::size_t i = 0; i < 3; ++i) delta[i] = detail.translation_new[i] - detail.translation_old[i];
        json["placement"] = QJsonObject{{"translation_old", toJsonArray(detail.translation_old)},
                                        {"translation_new", toJsonArray(detail.translation_new)},
                                        {"translation_delta", toJsonArray(delta)},
                                        {"moved", detail.moved},
                                        {"rotation_changed", detail.rotation_changed}};
    }
    if (!detail.openings_added.empty()) json["openings_added"] = toJsonArray(detail.openings_added);
    if (!detail.openings_removed.empty()) json["openings_removed"] = toJsonArray(detail.openings_removed);
    if (!detail.projections_added.empty()) json["projections_added"] = toJsonArray(detail.projections_added);
    if (!detail.projections_removed.empty()) json["projections_removed"] = toJsonArray(detail.projections_removed);
    if (detail.representation_changed) {
        QJsonArray items;
        for (const ItemChange& item : detail.items_changed) {
            QJsonObject attributes;
            for (const ItemAttributeChange& change : item.attributes) {
                attributes[QString::fromStdString(change.path)] =
                    QJsonObject{{"old", jsonValue(change.old_value)}, {"new", jsonValue(change.new_value)}};
            }
            items.append(QJsonObject{{"index", item.index},
                                     {"type_old", QString::fromStdString(item.type_old)},
                                     {"type_new", QString::fromStdString(item.type_new)},
                                     {"attributes", attributes}});
        }
        QJsonObject representation{{"representation_id_old", detail.representation_id_old},
                                   {"representation_id_new", detail.representation_id_new},
                                   {"items_changed", items}};
        if (detail.items_count_old != detail.items_count_new) {
            representation["items_count_old"] = detail.items_count_old;
            representation["items_count_new"] = detail.items_count_new;
        }
        json["representation"] = representation;
    }
    return json;
}

ElementRef refOf(const std::string& guid, const Entity& element) {
    return {guid, static_cast<int>(element.id()), util::className(element), nameOf(element)};
}

} // namespace

std::string changeKindName(PropertyChangeKind kind) {
    switch (kind) {
    case PropertyChangeKind::PropertyAdded: return "property added";
    case PropertyChangeKind::PropertyRemoved: return "property removed";
    case PropertyChangeKind::ValueAdded: return "value added";
    case PropertyChangeKind::ValueRemoved: return "value removed";
    case PropertyChangeKind::ValueChanged: return "value changed";
    }
    return {};
}

std::vector<std::string> ElementChange::kinds() const {
    std::vector<std::string> kinds;
    if (attributes_changed) kinds.push_back("attributes");
    if (geometry_changed) kinds.push_back("geometry");
    if (type_changed) kinds.push_back("type");
    if (properties_changed) kinds.push_back("property");
    if (container_changed) kinds.push_back("container");
    if (aggregate_changed) kinds.push_back("aggregate");
    if (classification_changed) kinds.push_back("classification");
    return kinds;
}

QJsonObject Result::toJson() const {
    QJsonArray added_json;
    for (const ElementRef& element : added) added_json.append(QString::fromStdString(element.global_id));
    QJsonArray deleted_json;
    for (const ElementRef& element : deleted) deleted_json.append(QString::fromStdString(element.global_id));
    QJsonObject changed_json;
    for (const auto& [guid, change] : changed) {
        QJsonObject entry;
        if (change.attributes_changed) {
            entry["attributes_changed"] = true;
            QJsonObject attributes;
            for (const AttributeChange& attribute : change.attributes) {
                attributes[QString::fromStdString(attribute.name)] =
                    QJsonObject{{"old", jsonValue(attribute.old_value)}, {"new", jsonValue(attribute.new_value)}};
            }
            entry["attributes"] = attributes;
        }
        if (change.geometry_changed) {
            entry["geometry_changed"] = true;
            entry["geometry"] = geometryJson(change.geometry);
        }
        if (change.type_changed) entry["type_changed"] = true;
        if (change.properties_changed) entry["properties_changed"] = propertiesJson(change.properties);
        if (change.container_changed) entry["container_changed"] = true;
        if (change.aggregate_changed) entry["aggregate_changed"] = true;
        if (change.classification_changed) entry["classification_changed"] = true;
        changed_json[QString::fromStdString(guid)] = entry;
    }
    return QJsonObject{{"added", added_json}, {"deleted", deleted_json}, {"changed", changed_json}};
}

Result diff(ifcopenshell::file& old_file, ifcopenshell::file& new_file, const Options& options) {
    Result result;
    result.precision = precisionOf(new_file);
    const double precision = result.precision;
    const bool shallow = options.is_shallow;
    const auto& relationships = options.relationships;
    const auto has = [&relationships](const char* name) { return relationships.count(name) > 0; };

    const auto old_elements = elementsByGuid(old_file);
    const auto new_elements = elementsByGuid(new_file);
    result.old_count = static_cast<int>(old_elements.size());
    result.new_count = static_cast<int>(new_elements.size());

    std::vector<std::pair<Entity, Entity>> common;
    for (const auto& [guid, element] : old_elements) {
        const auto found = new_elements.find(guid);
        if (found == new_elements.end()) result.deleted.push_back(refOf(guid, element));
        else common.emplace_back(element, found->second);
    }
    for (const auto& [guid, element] : new_elements) {
        if (!old_elements.count(guid)) result.added.push_back(refOf(guid, element));
    }

    std::optional<Shortlist> shortlist;
    if (options.ids && has("property")) shortlist = buildShortlist(*options.ids, old_file, new_file);

    ItemComparer comparer(precision, shallow);
    const std::size_t total = common.size();
    int last_percent = -1;
    for (std::size_t n = 0; n < total; ++n) {
        if (options.progress) {
            const int percent = total == 0 ? 100 : static_cast<int>(n * 100 / total);
            if (percent != last_percent) {
                last_percent = percent;
                options.progress(percent);
            }
        }
        const Entity& old_element = common[n].first;
        const Entity& new_element = common[n].second;
        ElementChange change;
        change.global_id = guidOf(new_element);
        change.old_id = static_cast<int>(old_element.id());
        change.new_id = static_cast<int>(new_element.id());
        change.ifc_class = util::className(new_element);
        change.name = nameOf(new_element);
        bool changed = false;
        const auto stop = [&]() { return shallow && changed; };

        if (has("attributes")) {
            change.attributes = diffAttributes(old_element, new_element, precision);
            change.attributes_changed = !change.attributes.empty();
            changed = change.attributes_changed;
        }
        if (!stop() && has("type")) {
            change.type_changed = !sameByGuid(util::get_type(old_element), util::get_type(new_element));
            changed = changed || change.type_changed;
        }
        if (!stop() && has("property")) {
            PropertyFilter keep;
            bool compare = true;
            if (shortlist) {
                const auto specs = shortlist->by_guid.find(change.global_id);
                compare = specs != shortlist->by_guid.end();
                if (compare) {
                    std::vector<const it::Facet*> facets;
                    for (std::size_t s : specs->second) {
                        for (const it::Facet& facet : shortlist->property_facets[s]) facets.push_back(&facet);
                    }
                    keep = [facets](const std::string& pset, const std::string& property) {
                        for (const it::Facet* facet : facets) {
                            if (facet->propertySet.matches(it::Value(pset)) && facet->baseName.matches(it::Value(property))) {
                                return true;
                            }
                        }
                        return false;
                    };
                }
            }
            if (compare) {
                try {
                    change.properties = diffProperties(get_psets(old_element), get_psets(new_element), precision, keep);
                } catch (...) {
                    change.properties.clear();
                }
                change.properties_changed = !change.properties.empty();
                changed = changed || change.properties_changed;
            }
        }
        if (!stop() && has("container")) {
            change.container_changed = !sameByGuid(util::get_container(old_element), util::get_container(new_element));
            changed = changed || change.container_changed;
        }
        if (!stop() && has("aggregate")) {
            change.aggregate_changed = !sameByGuid(util::get_aggregate(old_element), util::get_aggregate(new_element));
            changed = changed || change.aggregate_changed;
        }
        if (!stop() && has("classification")) {
            change.classification_changed = classificationReferences(old_element) != classificationReferences(new_element);
            changed = changed || change.classification_changed;
        }
        if (!stop() && has("geometry")) {
            try {
                change.geometry_changed = diffGeometry(old_element, new_element, precision, shallow, comparer, change.geometry);
            } catch (...) {
                change.geometry_changed = true;
            }
            changed = changed || change.geometry_changed;
        }
        if (changed && !change.global_id.empty()) result.changed.emplace(change.global_id, std::move(change));
    }
    if (options.progress) options.progress(100);
    return result;
}

} // namespace bonsaiviewer::services::ifcdiff
