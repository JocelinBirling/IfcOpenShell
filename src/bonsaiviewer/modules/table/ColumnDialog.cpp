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

#include "ColumnDialog.h"

#include "../property_selection/PropertyField.h"
#include "../../services/PopupService.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>

namespace bonsaiviewer::modules::table {

namespace {

const TableColumn::Kind kKinds[] = {
    TableColumn::Kind::Property, TableColumn::Kind::Name, TableColumn::Kind::Class,
    TableColumn::Kind::GlobalId, TableColumn::Kind::Model,
    TableColumn::Kind::Count, TableColumn::Kind::Color,
};

const TableColumn::Aggregation kAggregations[] = {
    TableColumn::Aggregation::Default, TableColumn::Aggregation::Concatenate,
    TableColumn::Aggregation::Sum,     TableColumn::Aggregation::Average,
    TableColumn::Aggregation::Min,     TableColumn::Aggregation::Max,
    TableColumn::Aggregation::Count,
};

} // namespace

ColumnDialog::ColumnDialog(const TableColumn& column, services::PropertyCatalog& catalog, bool is_new,
                           QWidget* parent)
    : components::Dialog(parent)
{
    setWindowTitle(is_new ? "Add Column" : "Modify Column");
    setModal(true);
    setMinimumWidth(620);

    auto* body = new QWidget(this);
    auto* form = new QFormLayout(body);
    form->setContentsMargins(0, 0, 0, 0);

    kind_combo_ = new QComboBox(body);
    for (TableColumn::Kind kind : kKinds) {
        kind_combo_->addItem(TableColumn::kindLabel(kind), static_cast<int>(kind));
    }
    kind_combo_->setCurrentIndex(kind_combo_->findData(static_cast<int>(column.kind)));
    form->addRow("Data", kind_combo_);

    property_field_ = new property_selection::PropertyField(catalog, body);
    property_field_->setValue(column.pset, column.property);
    form->addRow("Property", property_field_);

    title_edit_ = new QLineEdit(column.title, body);
    title_edit_->setPlaceholderText("Automatic (the property name)");
    form->addRow("Column name", title_edit_);

    auto_numeric_check_ = new QCheckBox("Convert texts that read as numbers to numbers (“.” or “,”)", body);
    auto_numeric_check_->setChecked(column.auto_numeric);
    form->addRow("AutoConvertTextToNumeric", auto_numeric_check_);

    aggregation_combo_ = new QComboBox(body);
    for (TableColumn::Aggregation aggregation : kAggregations) {
        aggregation_combo_->addItem(TableColumn::aggregationLabel(aggregation), static_cast<int>(aggregation));
    }
    aggregation_combo_->setCurrentIndex(aggregation_combo_->findData(static_cast<int>(column.aggregation)));
    form->addRow("Aggregation (grouped table)", aggregation_combo_);
    group_key_ = column.group_key;
    addBodyWidget(body);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ColumnDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    addFooterWidget(buttons);

    connect(kind_combo_, &QComboBox::currentIndexChanged, this, &ColumnDialog::updateKind);
    updateKind();
}

void ColumnDialog::updateKind() {
    const bool property = column().kind == TableColumn::Kind::Property;
    property_field_->setEnabled(property);
    auto_numeric_check_->setEnabled(property);
    // Count and Color rows are one value each: nothing to combine.
    const TableColumn::Kind kind = column().kind;
    aggregation_combo_->setEnabled(kind != TableColumn::Kind::Count && kind != TableColumn::Kind::Color);
    title_edit_->setPlaceholderText(property ? "Automatic (the property name)"
                                             : QString("Automatic (%1)").arg(kind_combo_->currentText()));
}

void ColumnDialog::setScopedSource(const QString& label, property_selection::PropertyPicker::ScopedLoader loader) {
    property_field_->setScopedSource(label, std::move(loader));
}

TableColumn ColumnDialog::column() const {
    TableColumn column;
    column.kind = static_cast<TableColumn::Kind>(kind_combo_->currentData().toInt());
    column.title = title_edit_->text().trimmed();
    column.aggregation = static_cast<TableColumn::Aggregation>(aggregation_combo_->currentData().toInt());
    column.group_key = group_key_;
    if (column.kind == TableColumn::Kind::Property) {
        column.pset = property_field_->propertySet();
        column.property = property_field_->propertyName();
        column.auto_numeric = auto_numeric_check_->isChecked();
    }
    return column;
}

void ColumnDialog::accept() {
    const TableColumn result = column();
    if (result.kind == TableColumn::Kind::Property && (result.pset.isEmpty() || result.property.isEmpty())) {
        services::popup::information(this, "A property column needs its property set and its property.");
        return;
    }
    QDialog::accept();
}

} // namespace bonsaiviewer::modules::table
