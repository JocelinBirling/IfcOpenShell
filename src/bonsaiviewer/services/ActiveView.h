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

#ifndef IFCINTERFACE_SERVICES_ACTIVEVIEW_H
#define IFCINTERFACE_SERVICES_ACTIVEVIEW_H

#include <cstdint>
#include <vector>

class ViewportWindow;
namespace bonsaiviewer { class SessionState; }

namespace bonsaiviewer::services {
class CategoryOverrideFilter;
class UndoStack;
}

// B02 — ActiveView (see 00_Technical_Implementation/BonsaiViewerTI00).
// The set of displayed elements of the current view: isolate / add /
// remove / showAll / hideAll / invert, expressed on whole element sets
// instead of the current selection. The viewport stays the authority for
// the hidden set; this service drives it and re-applies the
// CategoryOverrideFilter (B03) after every mutation so a disabled category
// can never come back through these paths.

namespace bonsaiviewer::services {

class ActiveView {
public:
    ActiveView(SessionState& session,
               ViewportWindow& viewport,
               CategoryOverrideFilter& categories);

    // Show only `ids` (everything else is hidden).
    void isolate(const std::vector<std::uint32_t>& ids);
    // Unhide `ids` (bring them back into the view).
    void add(const std::vector<std::uint32_t>& ids);
    // Hide `ids`.
    void remove(const std::vector<std::uint32_t>& ids);

    void showAll();
    void hideAll();
    void invert();

    // Visible = not hidden in the viewport AND allowed by the category
    // filter.
    bool isVisible(std::uint32_t object_id) const;
    std::vector<std::uint32_t> visibleIds() const;

    // Undo (BonsaiViewer27): the hidden set as it is now, and a way back.
    std::vector<std::uint32_t> hiddenIds() const;
    void restoreHidden(const std::vector<std::uint32_t>& hidden);

    // Set by the shell. Each mutation above records a display state first.
    void setUndoStack(UndoStack* undo) { undo_ = undo; }

private:
    // Every element of every open model, per the element registry.
    std::vector<std::uint32_t> allObjectIds() const;

    SessionState& session_;
    ViewportWindow& viewport_;
    CategoryOverrideFilter& categories_;
    UndoStack* undo_ = nullptr;
};

} // namespace bonsaiviewer::services

#endif
