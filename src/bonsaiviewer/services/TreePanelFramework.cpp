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

#include "TreePanelFramework.h"

#include "ContextMenuBuilder.h"

#include <QHeaderView>
#include <QIcon>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

#include <unordered_map>

namespace bonsaiviewer::services::tree {

namespace {

// The key travels with the item: data(0, UserRole) is the node key,
// data(1, UserRole+1) is the parent key for drop resolution.
constexpr int kKeyRole = Qt::UserRole;
constexpr int kFolderRole = Qt::UserRole + 1;
constexpr int kAcceptsChildrenRole = Qt::UserRole + 2;
constexpr int kDraggableRole = Qt::UserRole + 3;
constexpr int kExpandedRole = Qt::UserRole + 4;
constexpr int kVisibilityRole = Qt::UserRole + 5;

QTreeWidgetItem* makeItem(const TreeNode& node, QTreeWidgetItem* parent) {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, node.label);
    if (!node.icon.isEmpty()) item->setIcon(0, QIcon(node.icon));
    item->setData(0, kKeyRole, node.key);
    item->setData(0, kFolderRole, node.is_folder);
    item->setData(0, kAcceptsChildrenRole, node.accepts_children);
    item->setData(0, kDraggableRole, node.draggable);
    item->setFlags((node.draggable ? Qt::ItemIsDragEnabled : Qt::NoItemFlags)
                   | (node.accepts_children ? Qt::ItemIsDropEnabled : Qt::NoItemFlags)
                   | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (node.has_visibility) {
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(1, node.visibility ? Qt::Checked : Qt::Unchecked);
    }
    return item;
}

} // namespace

void TreeDataSource::onNodeClicked(const QString&) {}
void TreeDataSource::onNodeDoubleClicked(const QString&) {}
void TreeDataSource::onNodeMoved(const QString&, const QString&) {}

TreePanelWidget::TreePanelWidget(QTreeWidget* widget,
                                 TreeDataSource* data_source,
                                 QObject* parent)
    : QObject(parent), widget_(widget), data_source_(data_source) {
    widget_->setColumnCount(2);
    widget_->setHeaderHidden(true);
    widget_->setContextMenuPolicy(Qt::CustomContextMenu);
    widget_->setDragDropMode(QAbstractItemView::InternalMove);
    widget_->setSelectionMode(QAbstractItemView::SingleSelection);
    if (widget_->header()) {
        widget_->header()->setStretchLastSection(false);
        widget_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        widget_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    }

    QObject::connect(widget_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        data_source_->onNodeClicked(item->data(0, kKeyRole).toString());
    });
    QObject::connect(widget_, &QTreeWidget::itemDoubleClicked, this,
                     [this](QTreeWidgetItem* item, int) {
                         data_source_->onNodeDoubleClicked(item->data(0, kKeyRole).toString());
                     });
    // The checkbox is user state, not selection: intercept before the
    // click handler by reacting on the check state, not the click.
    QObject::connect(widget_, &QTreeWidget::itemChanged, this,
                     [this](QTreeWidgetItem* item, int column) {
                         if (column != 1) return;
                         // Re-entrancy guard: onVisibilityToggled may mutate
                         // the data source, which refresh()es.
                         const QSignalBlocker blocker(widget_);
                         data_source_->onVisibilityToggled(
                             item->data(0, kKeyRole).toString(),
                             item->checkState(1) == Qt::Checked);
                     });
    QObject::connect(widget_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        const QTreeWidgetItem* item = widget_->itemAt(pos);
        onContextMenu(widget_->viewport()->mapToGlobal(pos),
                      item ? item->data(0, kKeyRole).toString() : QString());
    });
    // Drag-and-drop: after Qt's InternalMove reorders the items, resolve
    // the new parent from the insert signal and hand the move to the
    // data source, which owns the real order; refresh() then rebuilds
    // from the source of truth.
    QObject::connect(widget_->model(), &QAbstractItemModel::rowsInserted, this,
                     [this](const QModelIndex& parent, int first, int last) {
        if (rebuilding_) return;
        const QString parent_key = parent.isValid()
            ? widget_->model()->data(parent, kKeyRole).toString()
            : QString();
        for (int row = first; row <= last; ++row) {
            const QModelIndex index = widget_->model()->index(row, 0, parent);
            const QString key = widget_->model()->data(index, kKeyRole).toString();
            if (!key.isEmpty()) data_source_->onNodeMoved(key, parent_key);
        }
        QMetaObject::invokeMethod(this, &TreePanelWidget::refresh, Qt::QueuedConnection);
    });
    QObject::connect(data_source_, &TreeDataSource::nodesChanged, this,
                      &TreePanelWidget::refresh);
}

void TreePanelWidget::refresh() {
    // Remember the expanded folders so a rebuild does not collapse the
    // tree under the user's cursor.
    std::unordered_map<QString, bool> expanded;
    for (QTreeWidgetItemIterator it(widget_); *it; ++it) {
        expanded[(*it)->data(0, kKeyRole).toString()] = (*it)->isExpanded();
    }
    const int top_index = widget_->indexOfTopLevelItem(widget_->currentItem());

    widget_->blockSignals(true);
    rebuilding_ = true;
    widget_->clear();
    for (const TreeNode& node : data_source_->topLevelNodes()) {
        QTreeWidgetItem* item = makeItem(node, nullptr);
        widget_->addTopLevelItem(item);
        if (node.is_folder) {
            buildChildren(node.key, item);
        }
    }
    rebuilding_ = false;
    widget_->blockSignals(false);

    for (QTreeWidgetItemIterator it(widget_); *it; ++it) {
        const auto found = expanded.find((*it)->data(0, kKeyRole).toString());
        if (found != expanded.end()) (*it)->setExpanded(found->second);
    }
    if (top_index >= 0 && top_index < widget_->topLevelItemCount()) {
        widget_->setCurrentItem(widget_->topLevelItem(top_index));
    }
}

void TreePanelWidget::buildChildren(const QString& parent_key, QTreeWidgetItem* parent_item) {
    for (const TreeNode& node : data_source_->childrenOf(parent_key)) {
        QTreeWidgetItem* item = makeItem(node, parent_item);
        if (node.is_folder) {
            buildChildren(node.key, item);
        }
    }
}

void TreePanelWidget::onContextMenu(const QPoint& global_pos, const QString& key) {
    ContextMenuBuilder menu;
    data_source_->populateContextMenu(key, menu);
    menu.exec(global_pos, widget_);
}

} // namespace bonsaiviewer::services::tree
