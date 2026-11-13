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

#include <QApplication>
#include <QHeaderView>
#include <QIcon>
#include <QSet>
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
constexpr int kExpandHintRole = Qt::UserRole + 6;
constexpr int kPlaceholderRole = Qt::UserRole + 7;

const QString kEyeOpen = QStringLiteral(":/icons/eye.svg");
const QString kEyeClosed = QStringLiteral(":/icons/eye-closed.svg");


} // namespace

QTreeWidgetItem* TreePanelWidget::makeItem(const TreeNode& node, QTreeWidgetItem* parent) const {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, node.label);
    if (!node.icon.isEmpty()) item->setIcon(0, QIcon(node.icon));
    if (!node.tooltip.isEmpty()) item->setToolTip(0, node.tooltip);
    item->setData(0, kKeyRole, node.key);
    item->setData(0, kFolderRole, node.is_folder);
    item->setData(0, kAcceptsChildrenRole, node.accepts_children);
    item->setData(0, kDraggableRole, node.draggable);
    item->setData(0, kExpandHintRole, node.expanded);
    item->setFlags((node.draggable ? Qt::ItemIsDragEnabled : Qt::NoItemFlags)
                   | (node.accepts_children ? Qt::ItemIsDropEnabled : Qt::NoItemFlags)
                   | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (node.has_visibility && eye_column_) {
        item->setData(1, kVisibilityRole, node.visibility);
        item->setIcon(1, QIcon(node.visibility ? kEyeOpen : kEyeClosed));
    } else if (node.has_visibility) {
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(1, node.visibility ? Qt::Checked : Qt::Unchecked);
    }
    if (lazy_ && node.is_folder) {
        auto* placeholder = new QTreeWidgetItem(item);
        placeholder->setData(0, kPlaceholderRole, true);
        placeholder->setFlags(Qt::NoItemFlags);
    }
    return item;
}

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

    QObject::connect(widget_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int column) {
        const QString key = item->data(0, kKeyRole).toString();
        const QVariant visible = item->data(1, kVisibilityRole);
        if (eye_column_ && column == 1 && visible.isValid()) {
            // The eye: toggle, not select. The data source may refresh().
            const bool now = !visible.toBool();
            item->setData(1, kVisibilityRole, now);
            item->setIcon(1, QIcon(now ? kEyeOpen : kEyeClosed));
            data_source_->onVisibilityToggled(key, now);
            return;
        }
        data_source_->onNodeClicked(key);
    });
    QObject::connect(widget_, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem* item) {
        saved_expanded_[item->data(0, kKeyRole).toString()] = true;
        if (lazy_) buildLazyChildren(item);
    });
    QObject::connect(widget_, &QTreeWidget::itemCollapsed, this, [this](QTreeWidgetItem* item) {
        saved_expanded_[item->data(0, kKeyRole).toString()] = false;
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
    // tree under the user's cursor. In lazy mode the folders never built
    // keep the state saved before.
    std::unordered_map<QString, bool> expanded = lazy_ ? saved_expanded_ : std::unordered_map<QString, bool>();
    for (QTreeWidgetItemIterator it(widget_); *it && !forget_expanded_; ++it) {
        if ((*it)->data(0, kPlaceholderRole).toBool()) continue;
        expanded[(*it)->data(0, kKeyRole).toString()] = (*it)->isExpanded();
    }
    forget_expanded_ = false;
    const int top_index = widget_->indexOfTopLevelItem(widget_->currentItem());
    // Lazy mode: the selected nodes stay selected (Hide / Show on several
    // nodes refreshes the tree).
    QSet<QString> selected;
    if (lazy_) {
        for (QTreeWidgetItem* item : widget_->selectedItems()) selected.insert(item->data(0, kKeyRole).toString());
    }

    widget_->blockSignals(true);
    rebuilding_ = true;
    widget_->clear();
    QList<QTreeWidgetItem*> top_items;
    for (const TreeNode& node : data_source_->topLevelNodes()) {
        QTreeWidgetItem* item = makeItem(node, nullptr);
        widget_->addTopLevelItem(item);
        top_items.append(item);
        if (node.is_folder && !lazy_) {
            buildChildren(node.key, item);
        }
    }
    rebuilding_ = false;
    widget_->blockSignals(false);

    if (lazy_) {
        saved_expanded_ = std::move(expanded);
        applyExpansion(top_items);
    } else {
        for (QTreeWidgetItemIterator it(widget_); *it; ++it) {
            const auto found = expanded.find((*it)->data(0, kKeyRole).toString());
            if (found != expanded.end()) (*it)->setExpanded(found->second);
        }
    }
    if (lazy_) {
        // Visual only: the data source still has its selection, and
        // setCurrentItem would replace the selection with one item.
        const QSignalBlocker blocker(widget_);
        for (QTreeWidgetItemIterator it(widget_); *it; ++it) {
            if (selected.contains((*it)->data(0, kKeyRole).toString())) (*it)->setSelected(true);
        }
    } else if (top_index >= 0 && top_index < widget_->topLevelItemCount()) {
        widget_->setCurrentItem(widget_->topLevelItem(top_index));
    }
}

void TreePanelWidget::applyExpansion(const QList<QTreeWidgetItem*>& items) {
    for (QTreeWidgetItem* item : items) {
        if (!item->data(0, kFolderRole).toBool()) continue;
        const auto found = saved_expanded_.find(item->data(0, kKeyRole).toString());
        const bool open = found != saved_expanded_.end() ? found->second : item->data(0, kExpandHintRole).toBool();
        // Expanding builds the children (itemExpanded), which apply theirs.
        if (open) item->setExpanded(true);
    }
}

void TreePanelWidget::buildLazyChildren(QTreeWidgetItem* parent_item) {
    if (parent_item->childCount() != 1 || !parent_item->child(0)->data(0, kPlaceholderRole).toBool()) return;
    QList<QTreeWidgetItem*> items;
    {
        const QSignalBlocker blocker(widget_);
        rebuilding_ = true;
        delete parent_item->takeChild(0);
        for (const TreeNode& node : data_source_->childrenOf(parent_item->data(0, kKeyRole).toString())) {
            items.append(makeItem(node, parent_item));
        }
        rebuilding_ = false;
    }
    applyExpansion(items);
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
    if (expand_menu_) {
        // The item is found again by its key: a refresh may replace it.
        const QTreeWidgetItem* item = key.isEmpty() ? nullptr : findItem(key);
        if (item && item->childCount() > 0) {
            menu.addSeparator();
            menu.addItem("expand-subtree", "Expand Subtree", [this, key]() {
                if (QTreeWidgetItem* found = findItem(key)) setSubtreeExpanded(found, true);
            });
            menu.addItem("collapse-subtree", "Collapse Subtree", [this, key]() {
                if (QTreeWidgetItem* found = findItem(key)) setSubtreeExpanded(found, false);
            });
        }
        if (!key.isEmpty()) menu.addSeparator();
        menu.addItem("expand-all", "Expand All", [this]() {
            for (int i = 0; i < widget_->topLevelItemCount(); ++i) setSubtreeExpanded(widget_->topLevelItem(i), true);
        });
        menu.addItem("collapse-all", "Collapse All", [this]() {
            for (int i = 0; i < widget_->topLevelItemCount(); ++i) setSubtreeExpanded(widget_->topLevelItem(i), false);
        });
    }
    menu.exec(global_pos, widget_);
}

QTreeWidgetItem* TreePanelWidget::findItem(const QString& key) const {
    for (QTreeWidgetItemIterator it(widget_); *it; ++it) {
        if ((*it)->data(0, kKeyRole).toString() == key) return *it;
    }
    return nullptr;
}

void TreePanelWidget::setSubtreeExpanded(QTreeWidgetItem* item, bool expanded) {
    if (!item->data(0, kFolderRole).toBool()) return;
    // A large tree takes a moment to build: the wait cursor, once.
    const bool outermost = !QApplication::overrideCursor();
    if (outermost && expanded) QApplication::setOverrideCursor(Qt::WaitCursor);
    // Expanding first builds the children (itemExpanded, lazy mode).
    if (expanded) item->setExpanded(true);
    for (int i = 0; i < item->childCount(); ++i) setSubtreeExpanded(item->child(i), expanded);
    if (!expanded) item->setExpanded(false);
    if (outermost && expanded) QApplication::restoreOverrideCursor();
}

} // namespace bonsaiviewer::services::tree
