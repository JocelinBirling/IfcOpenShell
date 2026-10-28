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

// PORT — see Attr.h.

#include "Attr.h"

#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"

namespace bonsaiviewer::ports::ifcopenshell_util {

namespace {

const ifcopenshell::entity* entityOf(const Entity& element) {
    return element ? element.declaration().as_entity() : nullptr;
}

} // namespace

bool isA(const Entity& element, const std::string& ifc_class) {
    return element && element.declaration().is(ifc_class);
}

std::string className(const Entity& element) {
    return element ? element.declaration().name() : std::string();
}

std::string schemaName(const Entity& element) {
    if (!element) return {};
    const auto* schema = element.declaration().schema();
    return schema ? schema->name() : std::string();
}

int attributeCategory(const Entity& element, const std::string& name) {
    const auto* entity = entityOf(element);
    if (!entity) return 0;
    if (entity->attribute_index(name) >= 0) return 1;
    for (const auto* attribute : entity->all_inverse_attributes()) {
        if (attribute->name() == name) return 2;
    }
    return 0;
}

std::optional<ifcopenshell::attribute_value> forward(const Entity& element, const std::string& name) {
    const auto* entity = entityOf(element);
    if (!entity) return std::nullopt;
    const auto index = entity->attribute_index(name);
    if (index < 0) return std::nullopt;
    auto value = element.get_attribute_value(static_cast<std::size_t>(index));
    if (value.isNull()) return std::nullopt;
    return value;
}

Entity forwardEntity(const Entity& element, const std::string& name) {
    const auto value = forward(element, name);
    if (!value || value->type() != ifcopenshell::Argument_ENTITY_INSTANCE) return {};
    return static_cast<express::base>(*value);
}

std::vector<Entity> forwardEntities(const Entity& element, const std::string& name) {
    const auto value = forward(element, name);
    if (!value || value->type() != ifcopenshell::Argument_AGGREGATE_OF_ENTITY_INSTANCE) return {};
    return static_cast<std::vector<express::base>>(*value);
}

std::optional<std::string> forwardString(const Entity& element, const std::string& name) {
    const auto value = forward(element, name);
    if (!value) return std::nullopt;
    if (value->type() == ifcopenshell::Argument_STRING) return static_cast<std::string>(*value);
    if (value->type() == ifcopenshell::Argument_ENUMERATION) {
        const ifcopenshell::enumeration_reference enumeration = *value;
        return std::string(enumeration.value() ? enumeration.value() : "");
    }
    return std::nullopt;
}

std::vector<Entity> inverse(const Entity& element, const std::string& name) {
    if (attributeCategory(element, name) != 2) return {};
    std::vector<Entity> result;
    for (const auto& item : element.as<express::entity>().get_inverse(name)) result.push_back(item);
    return result;
}

std::vector<std::string> forwardAttributeNames(const Entity& element) {
    std::vector<std::string> names;
    if (const auto* entity = entityOf(element)) {
        for (const auto* attribute : entity->all_attributes()) names.push_back(attribute->name());
    }
    return names;
}

ifcopenshell::attribute_value wrappedValue(const Entity& defined_type_instance) {
    return defined_type_instance.get_attribute_value(0);
}

} // namespace bonsaiviewer::ports::ifcopenshell_util
