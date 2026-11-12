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

#include "../../components/SvgIcon.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::bcf {

namespace {

QToolButton* toolButton(const QString& text, const QString& icon, const QString& tooltip, QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setIcon(components::icons::makeAccentSvgIcon(icon));
    button->setIconSize(QSize(16, 16));
    button->setText(text);
    button->setToolTip(tooltip);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setAutoRaise(true);
    return button;
}

} // namespace

BcfPanel::BcfPanel(QWidget* parent) : components::Panel("BCF", nullptr, parent) {
    auto* body = new QWidget(this);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(2);
    auto* load = toolButton("Load", ":/icons/folder.svg", "Open a BCF file (2.1 or 3.0)", body);
    save_button_ = toolButton("Save", ":/icons/floppy-disk.svg",
                              "Save the BCF file: choose the location and the version (2.1 or 3.0)", body);
    new_topic_button_ = toolButton("New Topic", ":/icons/plus.svg",
                                   "Create a topic: the active view (camera, selection, visibility, colours) "
                                   "and its image are captured",
                                   body);
    toolbar->addWidget(load);
    toolbar->addWidget(save_button_);
    toolbar->addWidget(new_topic_button_);
    toolbar->addStretch(1);
    layout->addLayout(toolbar);

    auto* filter_row = new QHBoxLayout();
    search_ = new QLineEdit(body);
    search_->setPlaceholderText("Search topic names (-fs: every field)");
    search_->setClearButtonEnabled(true);
    search_->setToolTip("Searches the topic names.\nStart with -fs for a full search: every field of the topic, "
                        "its comments included.");
    mode_ = new QComboBox(body);
    mode_->addItem("Table");
    mode_->addItem("Thumbnails");
    mode_->addItem("Tree");
    mode_->setToolTip("Table: one topic per row. Thumbnails: the images. Tree: each topic with its related topics.");
    filter_row->addWidget(search_, 1);
    filter_row->addWidget(mode_);
    layout->addLayout(filter_row);

    summary_ = new QLabel(body);
    summary_->setProperty("textRole", "secondary");
    layout->addWidget(summary_);

    stack_ = new QStackedWidget(body);
    tree_ = new QTreeWidget(stack_);
    tree_->setHeaderLabels({"Title", "Status", "Type", "Priority", "Assigned To", "Modified", "Comments"});
    tree_->setRootIsDecorated(false);
    tree_->setAlternatingRowColors(true);
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->setSortingEnabled(true);
    tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    thumbnails_ = new QListWidget(stack_);
    thumbnails_->setViewMode(QListView::IconMode);
    thumbnails_->setIconSize(QSize(160, 120));
    thumbnails_->setResizeMode(QListView::Adjust);
    thumbnails_->setWordWrap(true);
    thumbnails_->setSpacing(6);
    thumbnails_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    thumbnails_->setContextMenuPolicy(Qt::CustomContextMenu);
    stack_->addWidget(tree_);
    stack_->addWidget(thumbnails_);
    layout->addWidget(stack_, 1);
    addBodyWidget(body);

    connect(load, &QToolButton::clicked, this, &BcfPanel::loadRequested);
    connect(save_button_, &QToolButton::clicked, this, &BcfPanel::saveRequested);
    connect(new_topic_button_, &QToolButton::clicked, this, &BcfPanel::newTopicRequested);
    connect(search_, &QLineEdit::textChanged, this, &BcfPanel::searchChanged);
    connect(mode_, &QComboBox::currentIndexChanged, this, [this](int index) {
        stack_->setCurrentIndex(index == int(Mode::Thumbnails) ? 1 : 0);
        tree_->setRootIsDecorated(index == int(Mode::Tree));
        emit modeChanged();
    });
}

BcfPanel::Mode BcfPanel::mode() const {
    return static_cast<Mode>(mode_->currentIndex());
}

QString BcfPanel::searchText() const {
    return search_->text();
}

void BcfPanel::setSummary(const QString& text) {
    summary_->setText(text);
}

void BcfPanel::setFileActionsEnabled(bool enabled) {
    save_button_->setEnabled(enabled);
}

} // namespace bonsaiviewer::modules::bcf
