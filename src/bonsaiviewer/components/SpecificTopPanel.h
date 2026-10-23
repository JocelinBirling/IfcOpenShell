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

#ifndef IFCINTERFACE_COMPONENTS_SPECIFICTOPPANEL_H
#define IFCINTERFACE_COMPONENTS_SPECIFICTOPPANEL_H

#include <QFrame>

class QStackedWidget;

namespace bonsaiviewer::components {

// TI05 — SpecificTopPanel (see 00_Technical_Implementation/
// BonsaiViewerTI05). One instance per tab, below the generic top
// panel, giving the tab's own common functions. Content is registered
// by each feature (open subject of the spec, filled progressively);
// tabs without specific functions stay empty. setCurrentIndex follows
// the ribbon tab switch; a populated page is shown, an empty page
// collapses to nothing.

class SpecificTopPanel : public QFrame {
    Q_OBJECT

public:
    explicit SpecificTopPanel(QWidget* parent = nullptr);

    // One page per ribbon tab, in the same order; returns its index.
    int addTab(const QString& title);
    // Register a tab's content; null clears it (the page collapses).
    void setContent(int index, QWidget* widget);
    // Follows the ribbon tab switch.
    void setCurrentIndex(int index);

private:
    QStackedWidget* stack_ = nullptr;
};

} // namespace bonsaiviewer::components

#endif
