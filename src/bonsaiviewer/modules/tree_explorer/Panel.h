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

#ifndef IFCINTERFACE_MODULES_TREE_EXPLORER_PANEL_H
#define IFCINTERFACE_MODULES_TREE_EXPLORER_PANEL_H

#include "../../components/Panel.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;
class QTreeWidget;
class QWidget;

namespace bonsaiviewer::modules::tree_explorer {

// BonsaiViewer32 — the "Trees" panel: the tree selector, the scope, the
// search field, the banner of the interpreted trees, the Settings / Run row
// of the Unified tree, and the tree. The view (TreeExplorerView) fills it.
class TreeExplorerPanel : public components::Panel {
    Q_OBJECT
public:
    explicit TreeExplorerPanel(const QString& title, QWidget* parent = nullptr);

    QTreeWidget* tree() const { return tree_; }
    QComboBox* treeSelector() const { return selector_; }

    void setScopeEnabled(bool enabled);
    void setInterpreted(bool interpreted);
    // The Settings / Run row of the Unified tree.
    void setComputeRowVisible(bool visible);
    void setComputeStatus(const QString& text);
    void setComputeRunning(bool running);
    void clearSearch();

signals:
    void treeChosen(const QString& tree_id);
    void activeModelOnlyChanged(bool active_only);
    // Debounced while typing.
    void searchChanged(const QString& text);
    void computeSettingsRequested();
    void computeRunRequested();

private:
    QComboBox* selector_ = nullptr;
    QComboBox* scope_ = nullptr;
    QLineEdit* search_ = nullptr;
    QTimer* search_timer_ = nullptr;
    QLabel* banner_ = nullptr;
    QWidget* compute_row_ = nullptr;
    QPushButton* run_ = nullptr;
    QLabel* compute_status_ = nullptr;
    QTreeWidget* tree_ = nullptr;
};

} // namespace bonsaiviewer::modules::tree_explorer

#endif
