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
 ********************************************************************************/

#include "PropertyPicker.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::property_selection {

namespace {

constexpr int kPsetRole = Qt::UserRole;
constexpr int kPropertyRole = Qt::UserRole + 1;

enum Column {
    kNameColumn = 0,
    kCountColumn,
    kValuesColumn,
};

} // namespace

PropertyPicker::PropertyPicker(const bonsaiviewer::services::PropertyCatalog::PropertyMap& psets,
                               const bonsaiviewer::services::PropertyCatalog::SampleMap& samples,
                               const QString& scope,
                               QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Pick a Property");
    resize(620, 560);

    auto* layout = new QVBoxLayout(this);
    auto* scope_label = new QLabel(scope, this);
    scope_label->setProperty("textRole", "secondary");
    scope_label->setWordWrap(true);
    layout->addWidget(scope_label);

    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText("Search a property set, a property or a value…");
    search_edit_->setClearButtonEnabled(true);
    layout->addWidget(search_edit_);

    tree_ = new QTreeWidget(this);
    tree_->setColumnCount(3);
    tree_->setHeaderLabels({"Property", "Elements", "Values"});
    tree_->header()->setSectionResizeMode(kNameColumn, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(kCountColumn, QHeaderView::ResizeToContents);
    tree_->header()->setSectionResizeMode(kValuesColumn, QHeaderView::Stretch);
    tree_->setUniformRowHeights(true);
    tree_->setAlternatingRowColors(true);
    layout->addWidget(tree_, 1);

    // One group per property set, like the Properties panel.
    for (auto pset = psets.begin(); pset != psets.end(); ++pset) {
        auto* group = new QTreeWidgetItem(tree_);
        group->setText(kNameColumn, pset.key());
        group->setData(kNameColumn, kPsetRole, pset.key());
        QFont bold = group->font(kNameColumn);
        bold.setBold(true);
        group->setFont(kNameColumn, bold);
        group->setFirstColumnSpanned(true);
        const auto pset_samples = samples.value(pset.key());
        for (auto property = pset.value().begin(); property != pset.value().end(); ++property) {
            auto* item = new QTreeWidgetItem(group);
            item->setText(kNameColumn, property.key());
            item->setText(kCountColumn, QString::number(property.value()));
            item->setTextAlignment(kCountColumn, Qt::AlignRight | Qt::AlignVCenter);
            item->setText(kValuesColumn, pset_samples.value(property.key()).join(", "));
            item->setData(kNameColumn, kPsetRole, pset.key());
            item->setData(kNameColumn, kPropertyRole, property.key());
        }
    }
    tree_->expandAll();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    use_button_ = buttons->addButton("Use", QDialogButtonBox::AcceptRole);
    use_button_->setEnabled(false);
    layout->addWidget(buttons);

    connect(search_edit_, &QLineEdit::textChanged, this, &PropertyPicker::applyFilter);
    connect(tree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item, QTreeWidgetItem*) {
        use_button_->setEnabled(item != nullptr && !item->data(kNameColumn, kPropertyRole).toString().isEmpty());
    });
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        if (!item->data(kNameColumn, kPropertyRole).toString().isEmpty()) pickCurrent();
    });
    connect(use_button_, &QPushButton::clicked, this, &PropertyPicker::pickCurrent);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    search_edit_->setFocus();
}

void PropertyPicker::applyFilter(const QString& text) {
    const QString needle = text.trimmed();
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem* group = tree_->topLevelItem(i);
        const bool group_matches = group->text(kNameColumn).contains(needle, Qt::CaseInsensitive);
        int shown = 0;
        for (int j = 0; j < group->childCount(); ++j) {
            QTreeWidgetItem* item = group->child(j);
            const bool matches = needle.isEmpty() || group_matches ||
                                 item->text(kNameColumn).contains(needle, Qt::CaseInsensitive) ||
                                 item->text(kValuesColumn).contains(needle, Qt::CaseInsensitive);
            item->setHidden(!matches);
            shown += matches ? 1 : 0;
        }
        group->setHidden(shown == 0);
    }
}

void PropertyPicker::pickCurrent() {
    const QTreeWidgetItem* item = tree_->currentItem();
    if (item == nullptr) return;
    const QString property = item->data(kNameColumn, kPropertyRole).toString();
    if (property.isEmpty()) return;
    emit picked(item->data(kNameColumn, kPsetRole).toString(), property);
    accept();
}

} // namespace bonsaiviewer::modules::property_selection
