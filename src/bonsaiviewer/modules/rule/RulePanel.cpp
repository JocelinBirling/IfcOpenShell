// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or           *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                     *
 * but WITHOUT ANY WARRANTY; without the implied warranty of                     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 ********************************************************************************/

#include "RulePanel.h"

#include "Catalogue.h"

#include <QBrush>
#include <QColor>
#include <QDataStream>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::rule {

namespace {

// Rule name carried by an item-view drag: plain text when the source offers
// it, otherwise decoded from the standard internal item-model format.
QString draggedRuleName(const QMimeData* mime) {
    if (mime->hasText()) return mime->text();
    if (!mime->hasFormat("application/x-qabstractitemmodeldatalist")) return {};
    QByteArray encoded = mime->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    while (!stream.atEnd()) {
        int row = 0;
        int column = 0;
        QMap<int, QVariant> roles;
        stream >> row >> column >> roles;
        const QString text = roles.value(Qt::DisplayRole).toString();
        if (!text.isEmpty()) return text;
    }
    return {};
}

} // namespace

RuleTreeWidget::RuleTreeWidget(QWidget* parent) : QTreeWidget(parent) {
    setHeaderHidden(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);
    setExpandsOnDoubleClick(true);
}

RuleNodeType RuleTreeWidget::nodeType(const QTreeWidgetItem* item) {
    return static_cast<RuleNodeType>(item->data(0, Qt::UserRole).toInt());
}

QTreeWidgetItem* RuleTreeWidget::createRuleFile() {
    auto* item = new QTreeWidgetItem(this);
    item->setText(0, QString("Rule File #%1").arg(++file_counter_));
    item->setData(0, Qt::UserRole, int(RuleNodeType::File));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    expandItem(item);
    scrollToItem(item);
    return item;
}

QTreeWidgetItem* RuleTreeWidget::createFolder(QTreeWidgetItem* parent_item) {
    auto* item = parent_item ? new QTreeWidgetItem(parent_item) : new QTreeWidgetItem(this);
    item->setText(0, QString("Folder #%1").arg(++folder_counter_));
    item->setData(0, Qt::UserRole, int(RuleNodeType::Folder));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    if (parent_item) expandItem(parent_item);
    return item;
}

QTreeWidgetItem* RuleTreeWidget::createRuleItem(const QString& name) {
    auto* item = new QTreeWidgetItem;
    item->setText(0, name);
    item->setData(0, Qt::UserRole, int(RuleNodeType::Rule));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    if (const RuleDefinition* definition = findRuleDefinition(name)) {
        initRuleInstance(item, *definition);
        for (const QString& slot : definition->slot_names) addSlot(item, slot);
    }
    return item;
}

void RuleTreeWidget::addSlot(QTreeWidgetItem* rule_item, const QString& slot_name) {
    auto* slot = new QTreeWidgetItem(rule_item);
    slot->setText(0, slot_name);
    slot->setData(0, Qt::UserRole, int(RuleNodeType::Slot));
    slot->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
    slot->setForeground(0, QBrush(QColor(Qt::gray)));
}

void RuleTreeWidget::addRule(const QString& name, QTreeWidgetItem* container) {
    // A rule always lands inside a container; a drop on empty space gets a
    // fresh RuleFile so the "RuleFile holds 1..n rules" invariant holds.
    if (!container) container = createRuleFile();
    container->addChild(createRuleItem(name));
    expandItem(container);
}

void RuleTreeWidget::dragEnterEvent(QDragEnterEvent* event) {
    // External rule drags (from the Rule Library) are not understood by the
    // stock view, which would reject them before dropEvent runs.
    if (event->source() != this && !draggedRuleName(event->mimeData()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }
    QTreeWidget::dragEnterEvent(event);
}

void RuleTreeWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->source() != this && !draggedRuleName(event->mimeData()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }
    QTreeWidget::dragMoveEvent(event);
}

void RuleTreeWidget::dropEvent(QDropEvent* event) {
    if (event->source() == this) {
        // Internal drag: let Qt move the node (reorganise rules/folders).
        QTreeWidget::dropEvent(event);
        return;
    }
    const QString rule_name = draggedRuleName(event->mimeData());
    if (rule_name.isEmpty()) {
        event->ignore();
        return;
    }
    QTreeWidgetItem* target = itemAt(event->position().toPoint());
    if (target && nodeType(target) == RuleNodeType::Slot) {
        // Dropping on a placeholder fills it in place, keeping the role
        // label visible ("source : Overlap Check"); the filled rule then
        // exposes its own slots, if any.
        const QString slot_label = target->text(0);
        target->setData(0, kSlotNameRole, slot_label);
        target->setText(0, QString("%1 : %2").arg(slot_label, rule_name));
        target->setData(0, Qt::UserRole, int(RuleNodeType::Rule));
        target->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        target->setForeground(0, QBrush());
        if (const RuleDefinition* definition = findRuleDefinition(rule_name)) {
            initRuleInstance(target, *definition);
            for (const QString& slot : definition->slot_names) addSlot(target, slot);
        }
        expandItem(target);
        event->accept();
        return;
    }
    QTreeWidgetItem* container = nullptr;
    if (target) {
        // Dropping on a rule makes the new rule a sibling; dropping on a
        // RuleFile or a folder drops inside it.
        container = nodeType(target) == RuleNodeType::Rule ? target->parent() : target;
    }
    addRule(rule_name, container);
    event->accept();
}

RulePanel::RulePanel(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    outer->setSpacing(4);

    auto not_working = [this]() { QMessageBox::information(this, "Rule", "Not Working"); };

    auto* file_row = new QHBoxLayout();
    file_row->setSpacing(4);
    auto* save_button = new QPushButton("Save", this);
    auto* load_button = new QPushButton("Load", this);
    connect(save_button, &QPushButton::clicked, this, not_working);
    connect(load_button, &QPushButton::clicked, this, not_working);
    file_row->addWidget(save_button);
    file_row->addWidget(load_button);
    outer->addLayout(file_row);

    auto* create_row = new QHBoxLayout();
    create_row->setSpacing(4);
    auto* new_rule_file_button = new QPushButton("New RuleFile", this);
    auto* add_folder_button = new QPushButton("Add Folder", this);
    connect(new_rule_file_button, &QPushButton::clicked, this, [this]() { tree_->createRuleFile(); });
    connect(add_folder_button, &QPushButton::clicked, this, [this]() {
        QTreeWidgetItem* selected = tree_->currentItem();
        QTreeWidgetItem* parent_item = nullptr;
        if (selected) {
            parent_item = RuleTreeWidget::nodeType(selected) == RuleNodeType::Rule ? selected->parent() : selected;
        }
        tree_->createFolder(parent_item);
    });
    create_row->addWidget(new_rule_file_button);
    create_row->addWidget(add_folder_button);
    outer->addLayout(create_row);

    tree_ = new RuleTreeWidget(this);
    connect(tree_, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current, QTreeWidgetItem*) { emit ruleNodeSelected(current); });
    outer->addWidget(tree_, 1);
}

} // namespace bonsaiviewer::modules::rule
