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

#ifndef IFCINTERFACE_MODULES_TREE_EXPLORER_VIEW_H
#define IFCINTERFACE_MODULES_TREE_EXPLORER_VIEW_H

#include "IndexedSource.h"

#include <QObject>
#include <QString>

namespace bonsaiviewer::services::tree {
class TreePanelWidget;
}

namespace bonsaiviewer::modules::tree_explorer {

class TreeExplorerPanel;

// BonsaiViewer32 — drives one Trees panel: the data source of the chosen
// tree, wired to the panel's tree through the TreePanelFramework (lazy
// children, eye column), the scope and the search. The chosen tree is
// remembered per panel.
class TreeExplorerView : public QObject {
    Q_OBJECT
public:
    TreeExplorerView(TreeExplorerPanel* panel, const Context& context, QObject* parent = nullptr);

    void setTree(const QString& tree_id);
    QString treeId() const { return tree_id_; }

private:
    TreeExplorerPanel* panel_ = nullptr;
    Context context_;
    QString tree_id_;
    IndexedSource* source_ = nullptr;
    bonsaiviewer::services::tree::TreePanelWidget* tree_panel_ = nullptr;
    bool active_only_ = false;
    QString filter_;
};

} // namespace bonsaiviewer::modules::tree_explorer

#endif
