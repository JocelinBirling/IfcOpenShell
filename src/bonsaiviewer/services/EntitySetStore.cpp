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

#include "EntitySetStore.h"

#include "../ElementRegistry.h"
#include "../SessionState.h"

#include <algorithm>
#include <unordered_set>

namespace bonsaiviewer::services {

namespace {

std::vector<std::uint32_t> subtract(const std::vector<std::uint32_t>& base,
                                    const std::vector<std::uint32_t>& removed) {
    const std::unordered_set<std::uint32_t> gone(removed.begin(), removed.end());
    std::vector<std::uint32_t> out;
    out.reserve(base.size());
    for (std::uint32_t id : base) {
        if (gone.find(id) == gone.end()) out.push_back(id);
    }
    return out;
}

std::vector<std::uint32_t> unite(std::vector<std::uint32_t> base,
                                const std::vector<std::uint32_t>& added) {
    base.reserve(base.size() + added.size());
    for (std::uint32_t id : added) {
        if (std::find(base.begin(), base.end(), id) == base.end()) base.push_back(id);
    }
    return base;
}

} // namespace

EntitySetStore::EntitySetStore(bonsaiviewer::SessionState& session, QObject* parent)
    : QObject(parent), session_(session) {
    // A model set change may have dropped ids; a project reset empties
    // everything (spec: session only).
    connect(&session_, &bonsaiviewer::SessionState::modelsChanged, this,
            [this]() { purgeInvalidIds(); });
    connect(&session_, &bonsaiviewer::SessionState::projectReset, this, [this]() {
        current_.clear();
        for (auto& slot_ids : slots_) slot_ids.clear();
        emit currentChanged();
        for (int i = 1; i <= kSlotCount; ++i) emit slotChanged(i);
    });
}

void EntitySetStore::replaceCurrent(const std::vector<std::uint32_t>& ids) {
    current_ = ids;
    intersectValid(session_, current_);
    emit currentChanged();
}

void EntitySetStore::addToCurrent(const std::vector<std::uint32_t>& ids) {
    current_ = unite(std::move(current_), ids);
    intersectValid(session_, current_);
    emit currentChanged();
}

void EntitySetStore::removeFromCurrent(const std::vector<std::uint32_t>& ids) {
    current_ = subtract(current_, ids);
    emit currentChanged();
}

void EntitySetStore::saveSlot(int slot_index) {
    if (slot_index < 1 || slot_index > kSlotCount) return;
    slots_[slot_index - 1] = current_;
    emit slotChanged(slot_index);
}

const std::vector<std::uint32_t>& EntitySetStore::slot(int slot_index) const {
    static const std::vector<std::uint32_t> empty;
    if (slot_index < 1 || slot_index > kSlotCount) return empty;
    return slots_[slot_index - 1];
}

void EntitySetStore::purgeModel(std::uint32_t session_model_id) {
    std::vector<std::uint32_t> model_ids;
    for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
        model_ids.push_back(info.object_id);
    }
    const std::unordered_set<std::uint32_t> gone(model_ids.begin(), model_ids.end());
    const auto purge = [&gone](std::vector<std::uint32_t>& ids) {
        ids.erase(std::remove_if(ids.begin(), ids.end(),
                                 [&gone](std::uint32_t id) { return gone.count(id) != 0; }),
                  ids.end());
    };
    purge(current_);
    for (auto& slot_ids : slots_) purge(slot_ids);
    emit currentChanged();
    for (int i = 1; i <= kSlotCount; ++i) emit slotChanged(i);
}

void EntitySetStore::purgeInvalidIds() {
    const auto before = current_.size();
    intersectValid(session_, current_);
    for (auto& slot_ids : slots_) {
        const auto slot_before = slot_ids.size();
        intersectValid(session_, slot_ids);
        if (slot_ids.size() != slot_before) {
            emit slotChanged(int(&slot_ids - &slots_[0]) + 1);
        }
    }
    if (current_.size() != before) emit currentChanged();
}

void EntitySetStore::intersectValid(bonsaiviewer::SessionState& session,
                                    std::vector<std::uint32_t>& ids) {
    auto* registry = session.elementRegistry();
    ids.erase(std::remove_if(ids.begin(), ids.end(),
                             [registry](std::uint32_t id) {
                                 return !registry->findBasicElementInfo(id).has_value();
                             }),
              ids.end());
}

} // namespace bonsaiviewer::services
