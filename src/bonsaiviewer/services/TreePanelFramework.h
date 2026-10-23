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

#ifndef IFCINTERFACE_SERVICES_TREEPANELFRAMEWORK_H
#define IFCINTERFACE_SERVICES_TREEPANELFRAMEWORK_H

#include <QObject>
#include <QString>

#include <vector>

class QTreeWidget;
class QTreeWidgetItem;
class QWidget;

namespace bonsaiviewer::services {

class ContextMenuBuilder;

// B12 — TreePanelFramework (see 00_Technical_Implementation/
// BonsaiViewerTI00). A reusable tree panel parameterised by a
// TreeDataSource: the panel renders the nodes (label, optional icon,
// optional visibility column), handles click / double-click / context
// menu / drag-and-drop, and forwards everything to the data source. The
// conventions live in the data sources: click selects (B01),
// double-click isolates (B08). Consumers: the class tree (TI12), the
// filters (TI06), the table's Add Column window (TI07), the unified
// tree (TI13).

namespace tree {

struct TreeNode {
    // Stable id within the data source (object id, filter id, class
    // name, ...). Used for drag-and-drop targets and lookup.
    QString key;
    QString label;
    // Optional resource path (":/icons/...").
    QString icon;
    bool is_folder = false;
    // Folders can accept dropped nodes.
    bool accepts_children = false;
    bool draggable = true;
    // Second column with a toggle checkbox (visibility column).
    bool has_visibility = false;
    bool visibility = true;
};

class TreeDataSource : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

    virtual std::vector<TreeNode> topLevelNodes() = 0;
    // Children of a folder node; only called on folder nodes.
    virtual std::vector<TreeNode> childrenOf(const QString& key) = 0;

    // Populate the context menu of a node (items via the B15 builder).
    // Empty key is the menu on empty space.
    virtual void populateContextMenu(const QString& key, ContextMenuBuilder& menu) = 0;

    // Panel conventions. Click selects, double-click isolates — the
    // data source routes them through B01 / B08 itself.
    virtual void onNodeClicked(const QString& key);
    virtual void onNodeDoubleClicked(const QString& key);
    virtual void onVisibilityToggled(const QString& key, bool visible) = 0;
    // Drag-and-drop; empty new_parent is the root.
    virtual void onNodeMoved(const QString& key, const QString& new_parent);

signals:
    // The data changed; the panel rebuilds on the next refresh.
    void nodesChanged();
};

// The panel itself: a QTreeWidget wired to a data source. The tree is
// built lazily (children on expand) so large models stay cheap.
class TreePanelWidget : public QObject {
    Q_OBJECT

public:
    // `widget` stays owned by the caller; the panel only populates it.
    TreePanelWidget(QTreeWidget* widget, TreeDataSource* data_source, QObject* parent = nullptr);

    void refresh();

private:
    void buildChildren(const QString& parent_key, QTreeWidgetItem* parent_item);
    void onContextMenu(const QPoint& global_pos, const QString& key);

    QTreeWidget* widget_ = nullptr;
    TreeDataSource* data_source_ = nullptr;
    // True while refresh() populates the tree, so rowsInserted from the
    // population itself is not mistaken for a drag-and-drop move.
    bool rebuilding_ = false;
};

} // namespace bonsaiviewer::services::tree

} // namespace bonsaiviewer::services

#endif
