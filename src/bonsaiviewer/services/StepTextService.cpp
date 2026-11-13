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

#include "StepTextService.h"

#include "../ElementRegistry.h"
#include "../SessionState.h"
#include "../ifcparse/instance_data.h"
#include "../ifcviewer/SceneLoader.h"

#include <QRegularExpression>

#include <algorithm>
#include <sstream>

namespace bonsaiviewer::services {

namespace {

ifcopenshell::file* ifcFileOf(bonsaiviewer::SessionState& session,
                              std::uint32_t session_model_id) {
    if (!session.loader()) return nullptr;
    return session.loader()->ifcFile(session_model_id);
}

void addUsage(std::map<int, std::vector<int>>& index, int target_id, int source_id) {
    index[target_id].push_back(source_id);
}

// Reference ids held by one attribute value (entity, list of entities, list
// of lists of entities); the other argument types carry no reference.
void collectReferences(const ifcopenshell::attribute_value& attr, int source_id,
                       std::map<int, std::vector<int>>& index) {
    switch (attr.type()) {
    case ifcopenshell::Argument_ENTITY_INSTANCE: {
        const express::base ref = attr;
        if (ref) addUsage(index, static_cast<int>(ref.id()), source_id);
        break;
    }
    case ifcopenshell::Argument_AGGREGATE_OF_ENTITY_INSTANCE: {
        for (const express::base& ref : static_cast<std::vector<express::base>>(attr)) {
            if (ref) addUsage(index, static_cast<int>(ref.id()), source_id);
        }
        break;
    }
    case ifcopenshell::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE: {
        for (const auto& list : static_cast<std::vector<std::vector<express::base>>>(attr)) {
            for (const express::base& ref : list) {
                if (ref) addUsage(index, static_cast<int>(ref.id()), source_id);
            }
        }
        break;
    }
    default:
        break;
    }
}

} // namespace

StepTextService::StepTextService(bonsaiviewer::SessionState& session, QObject* parent)
    : QObject(parent), session_(session) {
    // The data source is immutable once loaded, so the usage index only
    // needs dropping when the model set changes.
    connect(&session_, &bonsaiviewer::SessionState::modelsChanged, this, [this]() {
        const QList<std::uint32_t> live = session_.sessionModelIds();
        QList<std::uint32_t> stale;
        for (auto it = usage_indexes_.keyBegin(); it != usage_indexes_.keyEnd(); ++it) {
            if (!live.contains(*it)) stale.append(*it);
        }
        for (std::uint32_t id : stale) dropModel(id);
    });
    connect(&session_, &bonsaiviewer::SessionState::projectReset, this,
            [this]() { usage_indexes_.clear(); });
}

QString StepTextService::lineFor(std::uint32_t session_model_id, int ifc_id) {
    ifcopenshell::file* file = ifcFileOf(session_, session_model_id);
    if (!file) return {};
    const express::base instance = file->instance_by_id(ifc_id);
    if (!instance) return {};
    std::ostringstream out;
    instance.to_string(out);
    return QString::fromStdString(out.str());
}

QString StepTextService::lineForObject(std::uint32_t object_id) {
    const auto info = session_.elementRegistry()->findBasicElementInfo(object_id);
    if (!info) return {};
    return lineFor(info->session_model_id, info->ifc_id);
}

std::vector<int> StepTextService::usagesOf(std::uint32_t session_model_id, int ifc_id) {
    if (!usage_indexes_.contains(session_model_id)) {
        buildUsageIndex(session_model_id);
    }
    const auto& index = usage_indexes_[session_model_id];
    const auto found = index.find(ifc_id);
    return found != index.end() ? found->second : std::vector<int>{};
}

std::vector<int> StepTextService::parseHashtags(const QString& line) {
    static const QRegularExpression re(QStringLiteral("#(\\d+)"));
    std::vector<int> ids;
    auto it = re.globalMatch(line);
    while (it.hasNext()) {
        const int id = it.next().captured(1).toInt();
        if (std::find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

bool StepTextService::modelAvailable(std::uint32_t session_model_id) const {
    if (!session_.loader()) return false;
    return session_.loader()->ifcFile(session_model_id) != nullptr;
}

void StepTextService::buildUsageIndex(std::uint32_t session_model_id) {
    ifcopenshell::file* file = ifcFileOf(session_, session_model_id);
    if (!file) {
        // Keep an empty entry so a missing source is not rescanned per
        // call; dropped when the model set changes.
        usage_indexes_.insert(session_model_id, {});
        return;
    }
    // Walk the attributes directly instead of serializing every instance to
    // a STEP line and re-parsing its hashtags: no string is built at all.
    std::map<int, std::vector<int>> index;
    for (auto it = file->begin(); it != file->end(); ++it) {
        const express::base instance = it->second;
        if (!instance) continue;
        const auto* decl = instance.declaration().as_entity();
        if (!decl) continue;
        const int source_id = static_cast<int>(it->first);
        const size_t attribute_count = decl->all_attributes().size();
        for (size_t i = 0; i < attribute_count; ++i) {
            collectReferences(instance.get_attribute_value(i), source_id, index);
        }
    }
    // Ascending, deduplicated users per target (one instance may reference
    // the same target from several attributes).
    for (auto& entry : index) {
        auto& users = entry.second;
        std::sort(users.begin(), users.end());
        users.erase(std::unique(users.begin(), users.end()), users.end());
    }
    usage_indexes_.insert(session_model_id, std::move(index));
}

void StepTextService::dropModel(std::uint32_t session_model_id) {
    usage_indexes_.remove(session_model_id);
}

} // namespace bonsaiviewer::services
