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

#include "IfcDataAccess.h"

#include "../ElementRegistry.h"
#include "element.h"

namespace bonsaiviewer::services {

IfcDataAccess::IfcDataAccess(ElementRegistry& registry)
    : registry_(registry) {}

std::optional<BasicElementInfo> IfcDataAccess::basicInfo(std::uint32_t object_id) const {
    return registry_.findBasicElementInfo(object_id);
}

std::optional<QString> IfcDataAccess::name(std::uint32_t object_id) const {
    const auto info = basicInfo(object_id);
    if (!info) {
        return std::nullopt;
    }
    return info->name;
}

std::optional<QString> IfcDataAccess::ifcClass(std::uint32_t object_id) const {
    const auto info = basicInfo(object_id);
    if (!info) {
        return std::nullopt;
    }
    return info->type;
}

std::optional<express::base> IfcDataAccess::entity(std::uint32_t object_id) const {
    return registry_.findEntity(object_id);
}

element_properties IfcDataAccess::propertySets(std::uint32_t object_id) const {
    const auto element = entity(object_id);
    if (!element) {
        return {};
    }
    return get_psets(*element, /*psets_only=*/true, /*qtos_only=*/false);
}

element_properties IfcDataAccess::quantitySets(std::uint32_t object_id) const {
    const auto element = entity(object_id);
    if (!element) {
        return {};
    }
    return get_psets(*element, /*psets_only=*/false, /*qtos_only=*/true);
}

std::vector<std::pair<std::string, std::string>> IfcDataAccess::scalarAttributes(std::uint32_t object_id) const {
    const auto element = entity(object_id);
    if (!element) {
        return {};
    }
    return get_scalar_attributes(*element);
}

PropertyIndex::PropertyIndex(ElementRegistry& registry, QObject* parent)
    : QObject(parent), registry_(registry) {}

void PropertyIndex::indexModel(std::uint32_t session_model_id) {
    ModelIndex& index = models_[session_model_id];
    index.fields.clear();
    for (const auto& info : registry_.basicElementInfoForModel(session_model_id)) {
        index.fields.emplace(info.object_id, std::make_pair(info.name, info.type));
    }
}

void PropertyIndex::removeModel(std::uint32_t session_model_id) {
    models_.remove(session_model_id);
}

std::vector<std::uint32_t> PropertyIndex::search(const QString& needle) const {
    if (needle.isEmpty()) {
        return {};
    }
    const QString lowered = needle.toLower();
    std::vector<std::uint32_t> result;
    for (auto it = models_.cbegin(); it != models_.cend(); ++it) {
        for (const auto& [object_id, fields] : it->fields) {
            if (fields.first.toLower().contains(lowered)
                || fields.second.toLower().contains(lowered)) {
                result.push_back(object_id);
            }
        }
    }
    return result;
}

std::vector<std::uint32_t> PropertyIndex::idsWithFieldValue(const QString& value) const {
    if (value.isEmpty()) {
        return {};
    }
    const QString lowered = value.toLower();
    std::vector<std::uint32_t> result;
    for (auto it = models_.cbegin(); it != models_.cend(); ++it) {
        for (const auto& [object_id, fields] : it->fields) {
            if (fields.first.toLower() == lowered || fields.second.toLower() == lowered) {
                result.push_back(object_id);
            }
        }
    }
    return result;
}

} // namespace bonsaiviewer::services
