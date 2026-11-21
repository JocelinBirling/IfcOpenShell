// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "Panel.h"

#include "../../components/Section.h"
#include "../../components/SvgIcon.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QToolButton>
#include <QTreeWidget>

namespace bonsaiviewer::modules::external_documents {

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

ExternalDocumentsPanel::ExternalDocumentsPanel(QWidget* parent)
    : components::Panel("External Documents", nullptr, parent)
{
    auto* section = new components::Section("", components::SectionHeaderMode::Hidden, this);
    section->setBodyExpanding(true);

    auto* toolbar = new QWidget(section);
    auto* toolbar_layout = new QHBoxLayout(toolbar);
    toolbar_layout->setContentsMargins(0, 0, 0, 0);
    toolbar_layout->setSpacing(2);
    auto* folder_button = makeToolButton("Folder", ":/icons/folder-plus.svg",
                                         "Add a folder (in the selected folder)", toolbar);
    auto* import_button = makeToolButton("Import", ":/icons/floppy-disk-arrow-in.svg",
                                         "Import external documents: tables (.xlsx/.csv) and 2D plans (.pdf)",
                                         toolbar);
    toolbar_layout->addWidget(folder_button);
    toolbar_layout->addWidget(import_button);
    toolbar_layout->addStretch(1);
    section->addBodyWidget(toolbar);

    tree_ = new QTreeWidget(section);
    tree_->setHeaderHidden(true);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setDragDropMode(QAbstractItemView::InternalMove);
    tree_->setDefaultDropAction(Qt::MoveAction);
    section->addBodyWidget(tree_);
    addBodyWidget(section);

    connect(folder_button, &QToolButton::clicked, this, &ExternalDocumentsPanel::addFolderRequested);
    connect(import_button, &QToolButton::clicked, this, &ExternalDocumentsPanel::importRequested);
}

} // namespace bonsaiviewer::modules::external_documents
