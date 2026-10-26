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
#include <QLineEdit>
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
    definition_.name = definition_.name.trimmed();
    QDialog::accept();
}

} // namespace bonsaiviewer::modules::table
