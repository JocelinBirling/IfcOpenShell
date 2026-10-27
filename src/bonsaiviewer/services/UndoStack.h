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

#ifndef IFCINTERFACE_SERVICES_UNDOSTACK_H
#define IFCINTERFACE_SERVICES_UNDOSTACK_H

#include "AppearanceService.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

// B27 — UndoStack (see BonsaiViewer27 CtrlZ). Keeps the last 10 display
// states: element visibility (ActiveView) and colour / opacity overrides
// (AppearanceService). One state is stored before each user action that
// changes the display. Section planes, camera and selection are not part of
// it. Filter runs are suspended and store nothing; undo itself stores nothing.

namespace bonsaiviewer::services {

class ActiveView;

class UndoStack {
public:
    static constexpr std::size_t kCapacity = 10;

    UndoStack(ActiveView& view, AppearanceService& appearance);

    // Stores the current display state. No-op while suspended.
    void record();
    // Puts back the most recent state. Returns false when there is none.
    bool undo();
    // Drops every state (project reset).
    void clear();

    void setSuspended(bool suspended) { suspended_ = suspended; }
    bool isSuspended() const { return suspended_; }
    std::size_t size() const { return states_.size(); }

private:
    struct DisplayState {
        std::vector<std::uint32_t> hidden_ids;
        AppearanceService::Snapshot appearance;
    };

    ActiveView& view_;
    AppearanceService& appearance_;
    std::deque<DisplayState> states_;
    bool suspended_ = false;
};

} // namespace bonsaiviewer::services

#endif
