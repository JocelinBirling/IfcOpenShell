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

#include "SelectionService.h"

#include "../SessionState.h"
#include "../ifcviewer/ViewportWindow.h"

namespace bonsaiviewer::services {

SelectionService::SelectionService(SessionState& session, ViewportWindow& viewport, QObject* parent)
    : QObject(parent), session_(session), viewport_(viewport) {}

void SelectionService::setSelection(const std::vector<std::uint32_t>& ids,
                                    SelectionSource source,
                                    SelectionMode mode) {
    const auto& current = viewport_.selection().selectionIds();
    std::unordered_set<std::uint32_t> next;
    if (mode == SelectionMode::Replace) {
        for (std::uint32_t id : ids) {
            if (id != 0) next.insert(id);
        }
    } else if (mode == SelectionMode::Add) {
        next = current;
        for (std::uint32_t id : ids) {
            if (id != 0) next.insert(id);
        }
    } else {
        next = current;
        for (std::uint32_t id : ids) {
            next.erase(id);
        }
    }
    viewport_.setSelectedObjectIds(std::vector<std::uint32_t>(next.begin(), next.end()));

    session_.setSelectedObjectId(viewport_.selection().activeId());
    session_.notifySelectionChanged();
    emit selectionChanged(selectionIdsAsVector(), source);
}

void SelectionService::onViewportPicked(std::uint32_t object_id) {
    session_.setSelectedObjectId(object_id);
    session_.notifySelectionChanged();
    emit selectionChanged(selectionIdsAsVector(), SelectionSource::Viewport);
}

const std::unordered_set<std::uint32_t>& SelectionService::selectionIds() const {
    return viewport_.selection().selectionIds();
}

std::vector<std::uint32_t> SelectionService::selectionIdsAsVector() const {
    const auto& ids = viewport_.selection().selectionIds();
    return std::vector<std::uint32_t>(ids.begin(), ids.end());
}

std::uint32_t SelectionService::activeObjectId() const {
    return viewport_.selection().activeId();
}

} // namespace bonsaiviewer::services
