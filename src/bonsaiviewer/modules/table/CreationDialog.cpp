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

#include "CreationDialog.h"

#include "../filters/SelectionEditor.h"
#include "../../services/PopupService.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScreen>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::table {

TableCreationDialog::TableCreationDialog(const TableDefinition& definition,
                                         services::IdsBridge& bridge,
                                         filters::IfcPathProvider ifc_path,
                                         QWidget* parent)
    : components::Dialog(parent), definition_(definition)
{
    setWindowTitle(definition_.name.isEmpty() ? "New Table" : "Modify Table");
    setModal(true);
    setMinimumSize(720, 480);
    if (const QScreen* screen = parent != nullptr ? parent->screen() : QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        resize(std::min(1040, available.width() * 9 / 10), std::min(720, available.height() * 9 / 10));
    }

    auto* body = new QWidget(this);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* identity = new QFormLayout();
    auto* name_edit = new QLineEdit(definition_.name, body);
    auto* description_edit = new QLineEdit(definition_.description, body);
    identity->addRow("Name", name_edit);
    identity->addRow("Description", description_edit);
    layout->addLayout(identity);
    connect(name_edit, &QLineEdit::textChanged, this, [this](const QString& text) { definition_.name = text; });
    connect(description_edit, &QLineEdit::textChanged, this,
            [this](const QString& text) { definition_.description = text; });

    auto* selection_box = new QGroupBox("Elements of the table (evaluated on all the loaded models)", body);
    auto* selection_layout = new QVBoxLayout(selection_box);
    selection_editor_ = new filters::SelectionEditor(bridge, std::move(ifc_path), selection_box);
    selection_editor_->setFields(definition_.selection.facets.empty()
                                     ? filters::SelectionEditor::defaultFields()
                                     : definition_.selection);
    selection_layout->addWidget(selection_editor_);
    layout->addWidget(selection_box, 1);
    connect(selection_editor_, &filters::SelectionEditor::isolateRequested,
            this, &TableCreationDialog::isolateRequested);

    // Grouping: the elements that share the values of the checked columns
    // form one row. Count and Color columns cannot be keys.
    auto* grouping_box = new QGroupBox("Grouping", body);
    auto* grouping_layout = new QVBoxLayout(grouping_box);
    grouping_layout->addWidget(new QLabel(
        "Checked columns are the keys: the elements sharing their values form one row. "
        "The other columns are combined with their aggregation.", grouping_box));
    grouping_list_ = new QListWidget(grouping_box);
    for (size_t index = 0; index < definition_.columns.size(); ++index) {
        const TableColumn& column = definition_.columns[index];
        if (column.kind == TableColumn::Kind::Count || column.kind == TableColumn::Kind::Color) continue;
        auto* item = new QListWidgetItem(column.displayTitle(), grouping_list_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(column.group_key ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, static_cast<qulonglong>(index));
    }
    grouping_layout->addWidget(grouping_list_);
    layout->addWidget(grouping_box);
    addBodyWidget(body);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &TableCreationDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    addFooterWidget(buttons);
}

void TableCreationDialog::accept() {
    definition_.selection = selection_editor_->fields();
    QString error;
    if (definition_.name.trimmed().isEmpty()) {
        error = "The table needs a name.";
    } else if (const QString selection_error = filters::SelectionEditor::validationError(definition_.selection);
               !selection_error.isEmpty()) {
        error = QString("Selection: %1.").arg(selection_error);
    }
    if (!error.isEmpty()) {
        services::popup::information(this, error);
        return;
    }
    for (int row = 0; row < grouping_list_->count(); ++row) {
        const QListWidgetItem* item = grouping_list_->item(row);
        definition_.columns[item->data(Qt::UserRole).toULongLong()].group_key =
            item->checkState() == Qt::Checked;
    }
    definition_.name = definition_.name.trimmed();
    QDialog::accept();
}

} // namespace bonsaiviewer::modules::table
