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

#include "TablesPanel.h"

#include "../../components/Section.h"
#include "../../components/SvgIcon.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace bonsaiviewer::modules::table {

namespace {

QToolButton* makeToolButton(const QString& text, const QString& icon_path,
                            const QString& tooltip, QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setIcon(components::icons::makeAccentSvgIcon(icon_path));
    button->setIconSize(QSize(16, 16));
    button->setText(text);
    button->setToolTip(tooltip);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setAutoRaise(true);
    return button;
}

} // namespace

TablesPanel::TablesPanel(QWidget* parent)
    : components::Panel("Tables", nullptr, parent)
{
    auto* section = new components::Section("", components::SectionHeaderMode::Hidden, this);
    section->setBodyExpanding(true);

    auto* toolbar = new QWidget(section);
    auto* toolbar_layout = new QHBoxLayout(toolbar);
    toolbar_layout->setContentsMargins(0, 0, 0, 0);
    toolbar_layout->setSpacing(2);
    auto* new_button = makeToolButton("Table", ":/icons/plus-square.svg",
                                      "Create a new table (in the selected folder)", toolbar);
    auto* folder_button = makeToolButton("Folder", ":/icons/folder.svg",
                                         "Add a folder (in the selected folder)", toolbar);
    run_button_ = makeToolButton("Run", ":/icons/refresh-double.svg",
                                 "Load the selected table and apply it to the loaded models", toolbar);
    auto* import_button = makeToolButton("Import", ":/icons/floppy-disk.svg",
                                         "Import a table file (in the selected folder)", toolbar);
    toolbar_layout->addWidget(new_button);
    toolbar_layout->addWidget(folder_button);
    toolbar_layout->addWidget(run_button_);
    toolbar_layout->addWidget(import_button);
    toolbar_layout->addStretch(1);
    section->addBodyWidget(toolbar);

    tree_ = new QTreeWidget(section);
    tree_->setHeaderHidden(true);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    section->addBodyWidget(tree_);
    addBodyWidget(section);

    connect(new_button, &QToolButton::clicked, this, &TablesPanel::newTableRequested);
    connect(folder_button, &QToolButton::clicked, this, &TablesPanel::addFolderRequested);
    connect(run_button_, &QToolButton::clicked, this, &TablesPanel::runRequested);
    connect(import_button, &QToolButton::clicked, this, &TablesPanel::importRequested);

    setRunEnabled(false);
}

QString TablesPanel::currentKey() const {
    const QTreeWidgetItem* item = tree_->currentItem();
    return item != nullptr ? item->data(0, Qt::UserRole).toString() : QString();
}

void TablesPanel::setRunEnabled(bool enabled) {
    run_button_->setEnabled(enabled);
}

} // namespace bonsaiviewer::modules::table
