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
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>
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
    all_psets_ = psets;
    all_samples_ = samples;
    scope_text_ = scope;
    auto* top = new QHBoxLayout();
    scope_label_ = new QLabel(scope, this);
    scope_label_->setProperty("textRole", "secondary");
    scope_label_->setWordWrap(true);
    top->addWidget(scope_label_, 1);
    scope_button_ = new QPushButton(this);
    scope_button_->setCheckable(true);
    scope_button_->setVisible(false);
    top->addWidget(scope_button_);
    layout->addLayout(top);

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
    fill(all_psets_, all_samples_);
    connect(scope_button_, &QPushButton::toggled, this, &PropertyPicker::showScoped);

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

void PropertyPicker::setScopedSource(const QString& label, ScopedLoader loader) {
    scoped_label_ = label;
    scoped_loader_ = std::move(loader);
    scope_button_->setText(QString("Only %1").arg(label));
    scope_button_->setToolTip("Show only the properties of these elements");
    scope_button_->setVisible(true);
    scope_button_->setChecked(true);  // toggled -> showScoped(true)
}

void PropertyPicker::showScoped(bool scoped) {
    if (!scoped) {
        scope_label_->setText(scope_text_);
        fill(all_psets_, all_samples_);
        return;
    }
    if (scoped_loaded_) {
        scope_label_->setText(QString("Properties of %1.").arg(scoped_label_));
        fill(scoped_psets_, scoped_samples_);
        return;
    }
    if (!scoped_loader_) return;
    scope_label_->setText(QString("Reading the properties of %1…").arg(scoped_label_));
    tree_->clear();
    QPointer<PropertyPicker> guard(this);
    scoped_loader_([this, guard](const PropertyMap& psets, const SampleMap& samples) {
        if (guard.isNull()) return;
        scoped_psets_ = psets;
        scoped_samples_ = samples;
        scoped_loaded_ = true;
        if (scope_button_->isChecked()) showScoped(true);
    });
}

void PropertyPicker::fill(const PropertyMap& psets, const SampleMap& samples) {
    tree_->clear();
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
    if (search_edit_ != nullptr) applyFilter(search_edit_->text());
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
