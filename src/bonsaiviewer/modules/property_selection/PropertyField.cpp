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

#include "PropertyField.h"

#include "PropertyPicker.h"

#include "../../services/PropertyCatalog.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTimer>

#include <algorithm>

namespace bonsaiviewer::modules::property_selection {

namespace {

// The completion text (what is matched and inserted); the display role
// carries the longer label shown in the popup.
constexpr int kCompletionRole = Qt::UserRole;
constexpr int kPsetRole = Qt::UserRole + 1;

constexpr int kVisibleProposals = 12;

QCompleter* makeCompleter(QStandardItemModel* model, QLineEdit* edit) {
    auto* completer = new QCompleter(model, edit);
    completer->setCompletionRole(kCompletionRole);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setMaxVisibleItems(kVisibleProposals);
    edit->setCompleter(completer);
    return completer;
}

int elementCount(const QMap<QString, int>& properties) {
    int count = 0;
    for (int value : properties) count = std::max(count, value);
    return count;
}

} // namespace

PropertyField::PropertyField(services::PropertyCatalog& catalog, QWidget* parent)
    : QWidget(parent), catalog_(catalog)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    pset_edit_ = new QLineEdit(this);
    pset_edit_->setPlaceholderText("Property set");
    pset_edit_->setClearButtonEnabled(true);
    property_edit_ = new QLineEdit(this);
    property_edit_->setPlaceholderText("Property");
    property_edit_->setClearButtonEnabled(true);
    browse_button_ = new QPushButton("Browse…", this);
    browse_button_->setToolTip("Pick a property among the property sets of the loaded models");
    layout->addWidget(pset_edit_, 1);
    layout->addWidget(property_edit_, 1);
    layout->addWidget(browse_button_);

    pset_model_ = new QStandardItemModel(this);
    property_model_ = new QStandardItemModel(this);
    pset_completer_ = makeCompleter(pset_model_, pset_edit_);
    property_completer_ = makeCompleter(property_model_, property_edit_);
    property_completer_->popup()->setMinimumWidth(360);

    connect(pset_edit_, &QLineEdit::textChanged, this, [this] {
        // Deferred: the completer may still be using the property model.
        QTimer::singleShot(0, this, &PropertyField::rebuildPropertyModel);
        emit changed();
    });
    connect(property_edit_, &QLineEdit::textChanged, this, &PropertyField::changed);
    connect(property_completer_, qOverload<const QModelIndex&>(&QCompleter::activated), this,
            [this](const QModelIndex& index) {
                const QString pset = index.data(kPsetRole).toString();
                if (!pset.isEmpty() && pset != pset_edit_->text()) pset_edit_->setText(pset);
            });
    connect(browse_button_, &QPushButton::clicked, this, &PropertyField::browse);
    connect(&catalog_, &services::PropertyCatalog::updated, this, [this] {
        rebuildPsetModel();
        rebuildPropertyModel();
    });

    catalog_.ensureBuilt();
    rebuildPsetModel();
    rebuildPropertyModel();
}

QString PropertyField::propertySet() const {
    return pset_edit_->text().trimmed();
}

QString PropertyField::propertyName() const {
    return property_edit_->text().trimmed();
}

void PropertyField::setValue(const QString& pset, const QString& property) {
    pset_edit_->setText(pset);
    property_edit_->setText(property);
}

void PropertyField::rebuildPsetModel() {
    pset_model_->clear();
    const auto& properties = catalog_.properties();
    for (const QString& pset : catalog_.propertySets()) {
        auto* item = new QStandardItem(
            QString("%1 — %2 element(s)").arg(pset).arg(elementCount(properties.value(pset))));
        item->setData(pset, kCompletionRole);
        pset_model_->appendRow(item);
    }
    pset_edit_->setToolTip(catalog_.isReady() ? QString()
                                              : "Reading the property sets of the loaded models…");
}

void PropertyField::rebuildPropertyModel() {
    property_model_->clear();
    const QString only_pset = propertySet();
    const auto& properties = catalog_.properties();
    const auto& samples = catalog_.samples();
    for (auto set = properties.cbegin(); set != properties.cend(); ++set) {
        if (!only_pset.isEmpty() && set.key() != only_pset) continue;
        for (auto property = set.value().cbegin(); property != set.value().cend(); ++property) {
            QString label = QString("%1 — %2").arg(property.key(), set.key());
            const QStringList values = samples.value(set.key()).value(property.key());
            if (!values.isEmpty()) label += " — " + values.join(", ");
            auto* item = new QStandardItem(label);
            item->setData(property.key(), kCompletionRole);
            item->setData(set.key(), kPsetRole);
            property_model_->appendRow(item);
        }
    }
}

void PropertyField::browse() {
    catalog_.ensureBuilt();
    const QString scope = catalog_.isReady()
        ? "Properties found on the elements of all the loaded models."
        : "The property sets are still being read: the list may be incomplete.";
    auto* picker = new PropertyPicker(catalog_.properties(), catalog_.samples(), scope, this);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    connect(picker, &PropertyPicker::picked, this, [this](const QString& pset, const QString& property) {
        setValue(pset, property);
    });
    picker->open();
}

} // namespace bonsaiviewer::modules::property_selection
