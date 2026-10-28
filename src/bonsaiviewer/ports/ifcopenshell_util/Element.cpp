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

// PORT — see Element.h. Each function quotes its Python name.

#include "Element.h"

namespace bonsaiviewer::ports::ifcopenshell_util {

namespace {

bool isIfc2x3(const Entity& element) {
    return schemaName(element) == "IFC2X3";
}

// getattr(entity, name, None) truthiness for an enumeration / string attribute;
// `...` (no such attribute) is told apart from None.
struct MaybeText {
    bool has_attribute = false;
    std::optional<std::string> value;
};

MaybeText getText(const Entity& element, const std::string& name) {
    MaybeText text;
    text.has_attribute = attributeCategory(element, name) == 1;
    if (text.has_attribute) text.value = forwardString(element, name);
    return text;
}

} // namespace

// def get_type(element)
Entity get_type(const Entity& element) {
    if (!element) return {};
    if (isA(element, "IfcTypeObject")) return element;
    if (!isIfc2x3(element)) {
        const auto is_typed_by = inverse(element, "IsTypedBy");
        if (!is_typed_by.empty()) return forwardEntity(is_typed_by.front(), "RelatingType");
        return {};
    }
    for (const Entity& relationship : inverse(element, "IsDefinedBy")) {  // IFC2X3
        if (isA(relationship, "IfcRelDefinesByType")) return forwardEntity(relationship, "RelatingType");
    }
    return {};
}

// def get_types(type)
std::vector<Entity> get_types(const Entity& type) {
    if (isIfc2x3(type)) {
        const auto object_type_of = inverse(type, "ObjectTypeOf");
        if (!object_type_of.empty()) return forwardEntities(object_type_of.front(), "RelatedObjects");
    } else {
        const auto types = inverse(type, "Types");
        if (!types.empty()) return forwardEntities(types.front(), "RelatedObjects");
    }
    return {};
}

// def get_predefined_type(element)
std::optional<std::string> get_predefined_type(const Entity& element) {
    if (const Entity element_type = get_type(element)) {
        std::optional<std::string> predefined_type = forwardString(element_type, "PredefinedType");
        if (predefined_type == "USERDEFINED" || !predefined_type || predefined_type->empty()) {
            const MaybeText element_type_text = getText(element_type, "ElementType");
            if (!element_type_text.has_attribute) {
                predefined_type = forwardString(element_type, "ProcessType");
            } else {
                predefined_type = element_type_text.value;
            }
        }
        if (predefined_type && !predefined_type->empty() && *predefined_type != "NOTDEFINED") return predefined_type;
    }
    std::optional<std::string> predefined_type = forwardString(element, "PredefinedType");
    if (predefined_type == "USERDEFINED" || !predefined_type || predefined_type->empty()) {
        predefined_type = forwardString(element, "ObjectType");
    }
    return predefined_type;
}

// def is_userdefined_type(element)
bool is_userdefined_type(const Entity& element) {
    if (const Entity element_type = get_type(element)) {
        std::optional<std::string> predefined_type = forwardString(element_type, "PredefinedType");
        if (predefined_type == "USERDEFINED") return true;
        if (!predefined_type || predefined_type->empty()) {
            const MaybeText element_type_text = getText(element_type, "ElementType");
            predefined_type = element_type_text.has_attribute ? element_type_text.value
                                                              : forwardString(element_type, "ProcessType");
            if (predefined_type && !predefined_type->empty()) return true;
        }
        if (predefined_type && !predefined_type->empty() && *predefined_type != "NOTDEFINED") return false;
    }
    const std::optional<std::string> predefined_type = forwardString(element, "PredefinedType");
    if (predefined_type == "USERDEFINED") return true;
    if (!predefined_type || predefined_type->empty()) {
        const auto object_type = forwardString(element, "ObjectType");
        return object_type && !object_type->empty();
    }
    return false;
}

// def get_aggregate(element)
Entity get_aggregate(const Entity& element) {
    const auto decomposes = inverse(element, "Decomposes");
    if (decomposes.empty()) return {};
    const Entity& rel = decomposes.front();
    if (isIfc2x3(element) && !isA(rel, "IfcRelAggregates")) {
        // In IFC2X3 Decomposes is used for both aggregates and nests.
        return {};
    }
    return forwardEntity(rel, "RelatingObject");
}

// def get_nest(element)
Entity get_nest(const Entity& element) {
    if (isIfc2x3(element)) {
        const auto decomposes = inverse(element, "Decomposes");
        if (decomposes.empty()) return {};
        if (isA(decomposes.front(), "IfcRelNests")) return forwardEntity(decomposes.front(), "RelatingObject");
        return {};
    }
    const auto nests = inverse(element, "Nests");
    if (!nests.empty()) return forwardEntity(nests.front(), "RelatingObject");
    return {};
}

// def get_container(element, should_get_direct=False, ifc_class=None)
Entity get_container(const Entity& element, bool should_get_direct, const std::string& ifc_class) {
    if (should_get_direct) {
        const auto contained_in_structure = inverse(element, "ContainedInStructure");
        if (!contained_in_structure.empty()) {
            const Entity container = forwardEntity(contained_in_structure.front(), "RelatingStructure");
            if (ifc_class.empty()) return container;
            if (isA(container, ifc_class)) return container;
        }
        return {};
    }
    const auto contained_in_structure = inverse(element, "ContainedInStructure");
    if (!contained_in_structure.empty()) {
        Entity container = forwardEntity(contained_in_structure.front(), "RelatingStructure");
        if (ifc_class.empty()) return container;
        while (container) {
            if (isA(container, ifc_class)) return container;
            container = get_aggregate(container);
        }
        return {};
    }
    if (const Entity parent = get_parent(element)) return get_container(parent, should_get_direct, ifc_class);
    return {};
}

// def get_filled_void(element)
Entity get_filled_void(const Entity& element) {
    const auto rel = inverse(element, "FillsVoids");
    return rel.empty() ? Entity() : forwardEntity(rel.front(), "RelatingOpeningElement");
}

// def get_voided_element(element)
Entity get_voided_element(const Entity& element) {
    const auto rel = inverse(element, "VoidsElements");
    return rel.empty() ? Entity() : forwardEntity(rel.front(), "RelatingBuildingElement");
}

// def get_adhered_element(element) — IFC4X3 and above.
Entity get_adhered_element(const Entity& element) {
    const auto rel = inverse(element, "AdheresToElement");
    return rel.empty() ? Entity() : forwardEntity(rel.front(), "RelatingElement");
}

// def get_parent(element, ifc_class=None)
Entity get_parent(const Entity& element, const std::string& ifc_class) {
    Entity parent = get_container(element, true);
    if (!parent) parent = get_aggregate(element);
    if (!parent) parent = get_nest(element);
    if (!parent) parent = get_filled_void(element);
    if (!parent) parent = get_voided_element(element);
    if (!parent) parent = get_adhered_element(element);
    if (ifc_class.empty()) return parent;
    while (parent) {
        if (isA(parent, ifc_class)) return parent;
        parent = get_parent(parent);
    }
    return {};
}

// def get_material(element, should_skip_usage=False, should_inherit=True)
Entity get_material(const Entity& element, bool should_skip_usage, bool should_inherit) {
    for (const Entity& relationship : inverse(element, "HasAssociations")) {
        if (!isA(relationship, "IfcRelAssociatesMaterial")) continue;
        const Entity relating_material = forwardEntity(relationship, "RelatingMaterial");
        if (should_skip_usage) {
            if (isA(relating_material, "IfcMaterialLayerSetUsage")) return forwardEntity(relating_material, "ForLayerSet");
            if (isA(relating_material, "IfcMaterialProfileSetUsage")) {
                return forwardEntity(relating_material, "ForProfileSet");
            }
        }
        return relating_material;
    }
    if (should_inherit) {
        const Entity relating_type = get_type(element);
        if (relating_type && relating_type != element && !inverse(relating_type, "HasAssociations").empty()) {
            return get_material(relating_type, should_skip_usage);
        }
    }
    return {};
}

} // namespace bonsaiviewer::ports::ifcopenshell_util
