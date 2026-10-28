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

#include "TableHeader.h"

#include "../../components/Section.h"
#include "../../components/SvgIcon.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSignalBlocker>
#include <QTableView>
#include <QToolButton>

namespace bonsaiviewer::modules::table {

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

TablePanel::TablePanel(QWidget* parent)
    : components::Panel("Table", nullptr, parent)
{
    auto* section = new components::Section("", components::SectionHeaderMode::Hidden, this);
    section->setBodyExpanding(true);

    auto* toolbar = new QWidget(section);
    auto* toolbar_layout = new QHBoxLayout(toolbar);
    toolbar_layout->setContentsMargins(0, 0, 0, 0);
    toolbar_layout->setSpacing(2);
    modify_button_ = makeToolButton("Modify", ":/icons/settings.svg",
                                    "Modify the name, the description and the elements of the table",
                                    false, toolbar);
    save_button_ = makeToolButton("Save", ":/icons/floppy-disk.svg",
                                  "Save the table configuration in the Tables panel", true, toolbar);
    column_button_ = makeToolButton("Column", ":/icons/plus-square.svg",
                                    "Add a column (right-most). Click a column header for its menu.",
                                    true, toolbar);
    refresh_button_ = makeToolButton("Refresh", ":/icons/refresh-double.svg",
                                     "Evaluate the table again on the loaded models", false, toolbar);
    export_button_ = makeToolButton("Export", ":/icons/floppy-disk.svg", "Export the table to a CSV file",
                                    false, toolbar);
    summary_label_ = new QLabel(toolbar);
    summary_label_->setProperty("textRole", "secondary");
    toolbar_layout->addWidget(modify_button_);
    toolbar_layout->addWidget(save_button_);
    toolbar_layout->addWidget(column_button_);
    toolbar_layout->addWidget(refresh_button_);
    toolbar_layout->addSpacing(8);
    toolbar_layout->addWidget(summary_label_, 1);
    toolbar_layout->addWidget(export_button_);
    section->addBodyWidget(toolbar);

    table_view_ = new QTableView(section);
    table_header_ = new TableHeader(table_view_);
    table_view_->setHorizontalHeader(table_header_);
    table_view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_view_->setAlternatingRowColors(true);
    table_view_->setWordWrap(false);
    table_view_->horizontalHeader()->setSectionsMovable(true);
    table_view_->horizontalHeader()->setSectionsClickable(true);
    table_view_->horizontalHeader()->setHighlightSections(false);
    table_view_->horizontalHeader()->setDefaultSectionSize(150);
    table_view_->verticalHeader()->setDefaultSectionSize(22);
    section->addBodyWidget(table_view_);
    addBodyWidget(section);

    connect(modify_button_, &QToolButton::clicked, this, &TablePanel::modifyTableRequested);
    connect(save_button_, &QToolButton::clicked, this, &TablePanel::saveRequested);
    connect(column_button_, &QToolButton::clicked, this, &TablePanel::addColumnRequested);
    connect(refresh_button_, &QToolButton::clicked, this, &TablePanel::refreshRequested);
    connect(export_button_, &QToolButton::clicked, this, &TablePanel::exportRequested);

    setSummary("No table loaded. Create or run one in the Tables panel.");
    setTableActionsEnabled(false);
}

void TablePanel::setSummary(const QString& text) {
    summary_label_->setText(text);
}

void TablePanel::setTableActionsEnabled(bool enabled) {
    for (QToolButton* button : {modify_button_, save_button_, column_button_, refresh_button_, export_button_}) {
        button->setEnabled(enabled);
    }
}

} // namespace bonsaiviewer::modules::table
