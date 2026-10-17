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
#include <QContextMenuEvent>
#include <QDataStream>
#include <QDropEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QMenu>
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

// The display name of an item without its "source : " slot prefix.
QString bareItemLabel(const QTreeWidgetItem* item) {
    const QString text = item->text(0);
    const int split = text.indexOf(" : ");
    return split >= 0 ? text.mid(split + 3) : text;
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

bool RuleTreeWidget::isFacetItem(const QTreeWidgetItem* item) {
    const QString key = item->data(0, kRuleKeyRole).toString();
    if (key.isEmpty()) return false;
    const RuleDefinition* definition = findRuleDefinition(key);
    return definition && definition->family == "IDS Facet";
}

QTreeWidgetItem* RuleTreeWidget::openRuleFile() const {
    return ruleFileItem();
}

QTreeWidgetItem* RuleTreeWidget::ruleFileItem() const {
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = const_cast<RuleTreeWidget*>(this)->topLevelItem(i);
        if (nodeType(item) == RuleNodeType::File) return item;
    }
    return nullptr;
}

QTreeWidgetItem* RuleTreeWidget::createRuleFile() {
    // SPEC 4.2: only one RuleFile open at a time — the new one replaces the
    // old, the whole tree is cleared.
    while (topLevelItemCount() > 0) {
        delete takeTopLevelItem(0);
    }
    auto* item = new QTreeWidgetItem(this);
    item->setText(0, QString("Rule File #%1").arg(++file_counter_));
    item->setData(0, Qt::UserRole, int(RuleNodeType::File));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    expandItem(item);
    scrollToItem(item);
    return item;
}

QTreeWidgetItem* RuleTreeWidget::createFolder(QTreeWidgetItem* parent_item) {
    // SPEC 4.2: a Folder always lives under a RuleFile, never at the root.
    if (!parent_item) {
        parent_item = ruleFileItem();
        if (!parent_item) parent_item = createRuleFile();
    }
    auto* item = new QTreeWidgetItem(parent_item);
    item->setText(0, QString("Folder #%1").arg(++folder_counter_));
    item->setData(0, Qt::UserRole, int(RuleNodeType::Folder));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    expandItem(parent_item);
    return item;
}

QTreeWidgetItem* RuleTreeWidget::createRuleItem(const QString& name) {
    // The dragged text is the definition's display label; the item's key
    // (kRuleKeyRole) is the stable catalogue name.
    const RuleDefinition* definition = resolveRuleDefinition(name);
    const QString label = definition ? definition->label : name;
    auto* item = new QTreeWidgetItem;
    item->setText(0, label);
    item->setData(0, Qt::UserRole, int(RuleNodeType::Rule));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    if (definition) {
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
    // A rule always lands inside a container; an empty-space drop reuses
    // the open RuleFile (only one at a time), creating it if needed.
    if (!container) {
        container = ruleFileItem();
        if (!container) container = createRuleFile();
    }
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
        // Internal drag. Dropping ON an existing rule or folder replaces
        // it (SPEC 4.4); anything else is the standard Qt reorganisation.
        QTreeWidgetItem* target = itemAt(event->position().toPoint());
        QTreeWidgetItem* dragged = currentItem();
        const bool dragged_is_facet = dragged && isFacetItem(dragged);
        const bool replace = target && dragged && target != dragged
            && dropIndicatorPosition() == QAbstractItemView::OnItem
            && (nodeType(target) == RuleNodeType::Rule || nodeType(target) == RuleNodeType::Folder)
            && nodeType(dragged) != RuleNodeType::File
            // SPEC 4.4: a facet may only replace a filled slot, never
            // take a place directly under a RuleFile or a Folder.
            && (!dragged_is_facet || !target->data(0, kSlotNameRole).toString().isEmpty());
        if (replace) {
            // A filled slot keeps its role label when replaced.
            const QString slot_label = target->data(0, kSlotNameRole).toString();
            if (!slot_label.isEmpty() && nodeType(dragged) == RuleNodeType::Rule) {
                dragged->setData(0, kSlotNameRole, slot_label);
                const QString bare_name = dragged->text(0);
                int split = bare_name.indexOf(" : ");
                const QString rule_name = split >= 0 ? bare_name.mid(split + 3) : bare_name;
                dragged->setText(0, QString("%1 : %2").arg(slot_label, rule_name));
                QFont font = dragged->font(0);
                font.setUnderline(true);
                dragged->setFont(0, font);
            }

            QTreeWidgetItem* old_parent = dragged->parent();
            if (old_parent) old_parent->removeChild(dragged);
            else takeTopLevelItem(indexOfTopLevelItem(dragged));

            QTreeWidgetItem* parent = target->parent();
            const int index = parent ? parent->indexOfChild(target)
                                    : indexOfTopLevelItem(target);
            delete target;
            if (parent) parent->insertChild(index, dragged);
            else insertTopLevelItem(index, dragged);
            event->accept();
            return;
        }
        if (dragged_is_facet) {
            // SPEC 4.4: an internally dragged facet may only fill an
            // empty slot in place; every other destination is refused.
            if (target && nodeType(target) == RuleNodeType::Slot
                && dropIndicatorPosition() == QAbstractItemView::OnItem) {
                QTreeWidgetItem* filled = fillSlot(
                    target, dragged->data(0, kRuleKeyRole).toString(),
                    dragged->data(0, kRuleValuesRole).toMap(),
                    dragged->data(0, kNodeIdRole).toString(),
                    dragged->data(0, kDescriptionRole).toString(),
                    bareItemLabel(dragged));
                if (filled) delete dragged;
                event->accept();
                return;
            }
            event->ignore();
            return;
        }
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
        // exposes its own slots, if any. A used slot is underlined.
        const QString slot_label = target->text(0);
        target->setData(0, kSlotNameRole, slot_label);
        target->setText(0, QString("%1 : %2").arg(slot_label, rule_name));
        target->setData(0, Qt::UserRole, int(RuleNodeType::Rule));
        target->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        target->setForeground(0, QBrush());
        QFont font = target->font(0);
        font.setUnderline(true);
        target->setFont(0, font);
        if (const RuleDefinition* definition = resolveRuleDefinition(rule_name)) {
            initRuleInstance(target, *definition);
            for (const QString& slot : definition->slot_names) addSlot(target, slot);
        }
        expandItem(target);
        event->accept();
        return;
    }
    // SPEC GeometryRule 4.4: an IDS Facet only ever lands in a slot, never
    // directly under a RuleFile or a Folder.
    if (const RuleDefinition* definition = resolveRuleDefinition(rule_name)) {
        if (definition->family == "IDS Facet") {
            event->ignore();
            return;
        }
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

QTreeWidgetItem* RuleTreeWidget::createRuleFileWithId(const QString& label, const QString& node_id) {
    QTreeWidgetItem* item = createRuleFile();
    item->setText(0, label);
    item->setData(0, kNodeIdRole, node_id);
    return item;
}

QTreeWidgetItem* RuleTreeWidget::createFolderWithId(QTreeWidgetItem* parent_item, const QString& label,
                                                   const QString& node_id, const QString& description) {
    QTreeWidgetItem* item = createFolder(parent_item);
    item->setText(0, label);
    item->setData(0, kNodeIdRole, node_id);
    item->setData(0, kDescriptionRole, description);
    return item;
}

QTreeWidgetItem* RuleTreeWidget::addRuleInstance(const QString& key, const QVariantMap& values,
                                                const QString& node_id, const QString& description,
                                                QTreeWidgetItem* container) {
    const RuleDefinition* definition = findRuleDefinition(key);
    if (!definition) return nullptr;
    if (!container) {
        container = ruleFileItem();
        if (!container) container = createRuleFile();
    }
    auto* item = createRuleItem(definition->label);
    initRuleInstance(item, *definition);
    item->setData(0, kRuleValuesRole, values);
    item->setData(0, kNodeIdRole, node_id);
    if (!description.isEmpty()) item->setData(0, kDescriptionRole, description);
    else item->setData(0, kDescriptionRole, definition->description);
    container->addChild(item);
    expandItem(container);
    return item;
}

QTreeWidgetItem* RuleTreeWidget::fillSlot(QTreeWidgetItem* slot_item, const QString& key,
                                          const QVariantMap& values, const QString& node_id,
                                          const QString& description, const QString& display_label) {
    const RuleDefinition* definition = findRuleDefinition(key);
    if (!definition || !slot_item) return nullptr;
    const QString slot_label = slot_item->text(0);
    slot_item->setData(0, kSlotNameRole, slot_label);
    slot_item->setText(0, QString("%1 : %2").arg(slot_label, display_label));
    slot_item->setData(0, Qt::UserRole, int(RuleNodeType::Rule));
    slot_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    slot_item->setForeground(0, QBrush());
    QFont font = slot_item->font(0);
    font.setUnderline(true);
    slot_item->setFont(0, font);
    initRuleInstance(slot_item, *definition);
    slot_item->setData(0, kRuleValuesRole, values);
    slot_item->setData(0, kNodeIdRole, node_id);
    slot_item->setData(0, kDescriptionRole,
                       description.isEmpty() ? definition->description : description);
    for (const QString& slot : definition->slot_names) addSlot(slot_item, slot);
    expandItem(slot_item);
    return slot_item;
}

QTreeWidgetItem* RuleTreeWidget::cloneTreeItem(const QTreeWidgetItem* source) {
    // Deep copy: every data role, the flags and the children (slots and
    // filled rules), so a pasted rule carries its whole configuration.
    auto* copy = new QTreeWidgetItem;
    const int roles[] = {Qt::UserRole, kRuleKeyRole, kRuleValuesRole, kDescriptionRole, kSlotNameRole, kNodeIdRole};
    for (int role : roles) {
        copy->setData(0, role, source->data(0, role));
    }
    copy->setText(0, source->text(0));
    copy->setFlags(source->flags());
    copy->setFont(0, source->font(0));
    for (int i = 0; i < source->childCount(); ++i) {
        copy->addChild(cloneTreeItem(source->child(i)));
    }
    return copy;
}

void RuleTreeWidget::contextMenuEvent(QContextMenuEvent* event) {
    QTreeWidgetItem* item = itemAt(event->pos());
    if (!item) return;

    QMenu menu(this);
    switch (nodeType(item)) {
    case RuleNodeType::File: {
        QAction* run = menu.addAction("Lancer le fichier de règle");
        connect(run, &QAction::triggered, this, [this, item]() { emit ruleRunRequested(item); });
        break;
    }
    case RuleNodeType::Folder: {
        QAction* run = menu.addAction("Lancer les règles du dossier");
        connect(run, &QAction::triggered, this, [this, item]() { emit ruleRunRequested(item); });
        QAction* paste = menu.addAction("Coller la Rule");
        paste->setEnabled(clipboard_ != nullptr);
        connect(paste, &QAction::triggered, this, [this, item]() {
            if (!clipboard_) return;
            item->addChild(cloneTreeItem(clipboard_));
            expandItem(item);
        });
        break;
    }
    case RuleNodeType::Rule: {
        if (isFacetItem(item)) {
            QAction* select_action =
                menu.addAction("Afficher la sélection en lançant l'entity");
            connect(select_action, &QAction::triggered, this,
                    [this, item]() { emit facetSelectionRequested(item); });
        } else {
            QAction* run = menu.addAction("Lancer la règle");
            connect(run, &QAction::triggered, this, [this, item]() { emit ruleRunRequested(item); });
            QAction* display = menu.addAction("Afficher le display Input");
            connect(display, &QAction::triggered, this,
                    [this, item]() { emit displayInputRequested(item); });
            QAction* copy_action =
                menu.addAction("Copier la rule, sa configuration et les emplacements");
            connect(copy_action, &QAction::triggered, this, [this, item]() {
                delete clipboard_;
                clipboard_ = cloneTreeItem(item);
            });
        }
        break;
    }
    case RuleNodeType::Slot:
        return;
    }
    // SPEC 4.5: Supprimer on every tree element — the item and all of
    // its children leave the tree (deleting the RuleFile empties it).
    QAction* remove = menu.addAction("Supprimer");
    connect(remove, &QAction::triggered, this, [this, item]() { delete item; });
    menu.exec(event->globalPos());
}

RulePanel::RulePanel(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    outer->setSpacing(4);

    auto* file_row = new QHBoxLayout();
    file_row->setSpacing(4);
    auto* save_button = new QPushButton("Save", this);
    auto* load_button = new QPushButton("Load", this);
    connect(save_button, &QPushButton::clicked, this, &RulePanel::saveRequested);
    connect(load_button, &QPushButton::clicked, this, &RulePanel::loadRequested);
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
    // Double-clicking a rule instance is the shortcut to run it (and fill
    // the Result panel); a facet double-click shows its selection.
    connect(tree_, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* item, int) {
        if (item && RuleTreeWidget::nodeType(item) == RuleNodeType::Rule) {
            emit tree_->ruleDoubleClicked(item);
        }
    });
    outer->addWidget(tree_, 1);
}

} // namespace bonsaiviewer::modules::rule
