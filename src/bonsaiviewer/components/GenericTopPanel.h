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

#ifndef IFCINTERFACE_COMPONENTS_GENERICTOPPANEL_H
#define IFCINTERFACE_COMPONENTS_GENERICTOPPANEL_H

#include <QFrame>

class QHBoxLayout;
class QWidget;

namespace bonsaiviewer::components {

// TI05 — GenericTopPanel (see 00_Technical_Implementation/
// BonsaiViewerTI05). A horizontal strip at the top of the 3D view,
// always visible whatever the active tab. Fixed slots: the search bar
// in the left corner (TI03), then the display functions, the box
// functions, the zoom functions and the colour functions of TI02/TI04.
// The panel is a dumb layout host — the shell (MainWindow) populates
// the groups so components stay free of service knowledge.

class GenericTopPanel : public QFrame {
    Q_OBJECT

public:
    explicit GenericTopPanel(QWidget* parent = nullptr);

    // The left-corner slot for the search bar.
    void setSearchBar(QWidget* bar);
    // The groups are appended left to right; the panel adds the final
    // stretch itself.
    QHBoxLayout* row() const { return row_; }
    // Spec 05: icon-only buttons (label in the tooltip), no group titles,
    // and no minimum width, so the strip never squeezes the side panels.
    // Call once the groups are populated.
    void makeCompact();

private:
    QHBoxLayout* row_ = nullptr;
};

} // namespace bonsaiviewer::components

#endif
