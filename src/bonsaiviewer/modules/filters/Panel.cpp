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

#include "Panel.h"

#include "../../components/Section.h"
#include "../../components/Style.h"
#include "../../components/SvgIcon.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QTreeWidget>

namespace bonsaiviewer::modules::filters {

namespace {

QToolButton* makeToolButton(const QString& text, const QString& icon_path,
                            const QString& tooltip, bool show_text, QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setIcon(components::icons::makeAccentSvgIcon(icon_path));
    button->setIconSize(QSize(16, 16));
    button->setText(text);
    button->setToolTip(tooltip);
    button->setToolButtonStyle(show_text ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
    button->setAutoRaise(true);
    return button;
}

} // namespace

FiltersPanel::FiltersPanel(QWidget* parent)
    : components::Panel("Filters", nullptr, parent)
{
    auto* section = new components::Section("", components::SectionHeaderMode::Hidden, this);
    section->setBodyExpanding(true);

    // Toolbar: the creation actions are labelled, the others icon-only.
    auto* toolbar = new QWidget(section);
    auto* toolbar_layout = new QHBoxLayout(toolbar);
    toolbar_layout->setContentsMargins(0, 0, 0, 0);
    toolbar_layout->setSpacing(2);
    auto* add_filter = makeToolButton("Filter", ":/icons/plus-square.svg",
                                      "Add a filter (in the selected folder)", true, toolbar);
    auto* add_folder = makeToolButton("Folder", ":/icons/folder-plus.svg",
                                      "Add a folder (in the selected folder)", true, toolbar);
    run_button_ = makeToolButton("Run", ":/icons/check.svg",
                                 "Run the selected filter on the whole model (or double-click it)",
                                 false, toolbar);
    run_box_button_ = makeToolButton("Run on the Box", ":/icons/box-arrow-in.svg",
                                     "Run the selected filter on the elements of the box", false, toolbar);
    auto* import_button = makeToolButton("Import", ":/icons/download-square.svg",
                                         "Import filters (one filter or a whole library)", false, toolbar);
    auto* export_button = makeToolButton("Export", ":/icons/floppy-disk.svg",
                                         "Export all the filters to a library file", false, toolbar);
    toolbar_layout->addWidget(add_filter);
    toolbar_layout->addWidget(add_folder);
    toolbar_layout->addSpacing(6);
    toolbar_layout->addWidget(run_button_);
    toolbar_layout->addWidget(run_box_button_);
    toolbar_layout->addStretch(1);
    toolbar_layout->addWidget(import_button);
    toolbar_layout->addWidget(export_button);
    section->addBodyWidget(toolbar);

    empty_hint_ = new QLabel("No filter yet. Click “Filter” to create one, "
                             "or “Import” to load filters shared by someone else.",
                             section);
    empty_hint_->setWordWrap(true);
    empty_hint_->setProperty("textRole", "secondary");
    section->addBodyWidget(empty_hint_);

    tree_ = new QTreeWidget(section);
    tree_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    section->addBodyWidget(tree_);
    addBodyWidget(section);

    connect(add_filter, &QToolButton::clicked, this, &FiltersPanel::addFilterRequested);
    connect(add_folder, &QToolButton::clicked, this, &FiltersPanel::addFolderRequested);
    connect(run_button_, &QToolButton::clicked, this, &FiltersPanel::runRequested);
    connect(run_box_button_, &QToolButton::clicked, this, &FiltersPanel::runOnBoxRequested);
    connect(import_button, &QToolButton::clicked, this, &FiltersPanel::importRequested);
    connect(export_button, &QToolButton::clicked, this, &FiltersPanel::exportRequested);

    setRunEnabled(false);
}

QString FiltersPanel::currentKey() const {
    const QTreeWidgetItem* item = tree_->currentItem();
    return item != nullptr ? item->data(0, Qt::UserRole).toString() : QString();
}

void FiltersPanel::setRunEnabled(bool enabled) {
    run_button_->setEnabled(enabled);
    run_box_button_->setEnabled(enabled);
}

void FiltersPanel::setEmptyHintVisible(bool visible) {
    empty_hint_->setVisible(visible);
}

} // namespace bonsaiviewer::modules::filters
