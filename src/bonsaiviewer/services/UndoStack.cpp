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

#include "UndoStack.h"

#include "ActiveView.h"

#include <utility>

namespace bonsaiviewer::services {

UndoStack::UndoStack(ActiveView& view, AppearanceService& appearance)
    : view_(view), appearance_(appearance) {}

void UndoStack::record() {
    if (suspended_) return;
    states_.push_back(DisplayState{view_.hiddenIds(), appearance_.snapshot()});
    while (states_.size() > kCapacity) {
        states_.pop_front();
    }
}

bool UndoStack::undo() {
    if (states_.empty()) return false;
    DisplayState state = std::move(states_.back());
    states_.pop_back();
    view_.restoreHidden(state.hidden_ids);
    appearance_.restore(state.appearance);
    return true;
}

void UndoStack::clear() {
    states_.clear();
}

} // namespace bonsaiviewer::services
