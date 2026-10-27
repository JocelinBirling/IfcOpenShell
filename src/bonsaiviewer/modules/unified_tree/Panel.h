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

#ifndef IFCINTERFACE_MODULES_UNIFIED_TREE_PANEL_H
#define IFCINTERFACE_MODULES_UNIFIED_TREE_PANEL_H

#include "../../components/Panel.h"

class QLabel;
class QPushButton;
class QTreeWidget;

namespace bonsaiviewer::modules::unified_tree {

// BonsaiViewer13 — the Unified Tree panel: a banner saying the data is
// deduced, the Run button, a status line and the tree. The Settings button is
// the panel's own (settingsRequested). The view fills the tree.
class UnifiedTreePanel : public components::Panel {
    Q_OBJECT
public:
    explicit UnifiedTreePanel(QWidget* parent = nullptr);

    QTreeWidget* tree() const { return tree_; }
    void setStatus(const QString& text);
    void setRunning(bool running);

signals:
    void runRequested();

private:
    QTreeWidget* tree_ = nullptr;
    QLabel* status_ = nullptr;
    QPushButton* run_ = nullptr;
};

} // namespace bonsaiviewer::modules::unified_tree

#endif
