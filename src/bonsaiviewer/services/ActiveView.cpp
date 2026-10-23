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

#include "ActiveView.h"

#include "CategoryOverrideFilter.h"
#include "../ElementRegistry.h"
#include "../SessionState.h"
#include "../ifcviewer/ViewportWindow.h"

#include <unordered_set>

namespace bonsaiviewer::services {

ActiveView::ActiveView(SessionState& session,
                       ViewportWindow& viewport,
                       CategoryOverrideFilter& categories)
    : session_(session), viewport_(viewport), categories_(categories) {}

void ActiveView::isolate(const std::vector<std::uint32_t>& ids) {
    std::unordered_set<std::uint32_t> wanted(ids.begin(), ids.end());
    std::vector<std::uint32_t> complement;
    for (std::uint32_t id : allObjectIds()) {
        if (wanted.find(id) == wanted.end()) {
            complement.push_back(id);
        }
    }
    viewport_.setObjectsVisible(complement, false);
    viewport_.setObjectsVisible(ids, true);
    categories_.applyToViewport();
    session_.notifyVisibilityChanged();
}

void ActiveView::add(const std::vector<std::uint32_t>& ids) {
    viewport_.setObjectsVisible(ids, true);
    categories_.applyToViewport();
    session_.notifyVisibilityChanged();
}

void ActiveView::remove(const std::vector<std::uint32_t>& ids) {
    viewport_.setObjectsVisible(ids, false);
    session_.notifyVisibilityChanged();
}

void ActiveView::showAll() {
    viewport_.showAllElements();
    categories_.applyToViewport();
    session_.notifyVisibilityChanged();
}

void ActiveView::hideAll() {
    viewport_.hideAllElements();
    session_.notifyVisibilityChanged();
}

void ActiveView::invert() {
    viewport_.invertElementVisibility();
    categories_.applyToViewport();
    session_.notifyVisibilityChanged();
}

bool ActiveView::isVisible(std::uint32_t object_id) const {
    return !viewport_.isObjectHidden(object_id) && categories_.isObjectAllowed(object_id);
}

std::vector<std::uint32_t> ActiveView::visibleIds() const {
    std::vector<std::uint32_t> ids;
    for (std::uint32_t id : allObjectIds()) {
        if (isVisible(id)) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::vector<std::uint32_t> ActiveView::allObjectIds() const {
    std::vector<std::uint32_t> ids;
    for (std::uint32_t session_model_id : session_.sessionModelIds()) {
        for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
            ids.push_back(info.object_id);
        }
    }
    return ids;
}

} // namespace bonsaiviewer::services
