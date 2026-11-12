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

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::tree_explorer {

TreeExplorerPanel::TreeExplorerPanel(const QString& title, QWidget* parent)
    : components::Panel(title, nullptr, parent)
{
    auto* section = new components::Section("", components::SectionHeaderMode::Hidden, this);
    section->setBodyExpanding(true);

    auto* body = new QWidget(section);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // The tree selector.
    auto* top = new QWidget(body);
    auto* top_layout = new QHBoxLayout(top);
    top_layout->setContentsMargins(0, 0, 0, 0);
    selector_ = new QComboBox(top);
    selector_->setToolTip("The tree shown in this panel");
    top_layout->addWidget(selector_, 1);
    layout->addWidget(top);

    search_ = new QLineEdit(body);
    search_->setPlaceholderText("Search by name");
    search_->setClearButtonEnabled(true);
    layout->addWidget(search_);

    banner_ = new QLabel("Deduced data: not read from the models.", body);
    banner_->setObjectName("treeExplorerBanner");
    banner_->setWordWrap(true);
    banner_->setStyleSheet("QLabel#treeExplorerBanner { background: rgba(255, 190, 60, 60);"
                           " border: 1px solid rgba(255, 190, 60, 160); border-radius: 4px; padding: 4px 6px; }");
    banner_->setVisible(false);
    layout->addWidget(banner_);

    compute_row_ = new QWidget(body);
    auto* compute_layout = new QHBoxLayout(compute_row_);
    compute_layout->setContentsMargins(0, 0, 0, 0);
    auto* settings = new QPushButton("Settings", compute_row_);
    settings->setToolTip("The reference models, the tolerance and the merges");
    run_ = new QPushButton("Run", compute_row_);
    run_->setToolTip("Compute the tree with the current settings");
    compute_status_ = new QLabel(compute_row_);
    compute_status_->setProperty("textRole", "secondary");
    compute_status_->setWordWrap(true);
    compute_layout->addWidget(settings);
    compute_layout->addWidget(run_);
    compute_layout->addWidget(compute_status_, 1);
    compute_row_->setVisible(false);
    layout->addWidget(compute_row_);

    tree_ = new QTreeWidget(body);
    tree_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tree_->setIconSize(QSize(16, 16));
    tree_->setUniformRowHeights(true);
    layout->addWidget(tree_, 1);

    section->addBodyWidget(body);
    addBodyWidget(section);

    search_timer_ = new QTimer(this);
    search_timer_->setSingleShot(true);
    search_timer_->setInterval(250);
    connect(search_, &QLineEdit::textChanged, search_timer_, qOverload<>(&QTimer::start));
    connect(search_timer_, &QTimer::timeout, this, [this]() { emit searchChanged(search_->text()); });
    connect(selector_, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        emit treeChosen(selector_->itemData(index).toString());
    });
    connect(settings, &QPushButton::clicked, this, &TreeExplorerPanel::computeSettingsRequested);
    connect(run_, &QPushButton::clicked, this, &TreeExplorerPanel::computeRunRequested);
}

void TreeExplorerPanel::setInterpreted(bool interpreted) {
    banner_->setVisible(interpreted);
}

void TreeExplorerPanel::setComputeRowVisible(bool visible) {
    compute_row_->setVisible(visible);
}

void TreeExplorerPanel::setComputeStatus(const QString& text) {
    compute_status_->setText(text);
}

void TreeExplorerPanel::setComputeRunning(bool running) {
    run_->setEnabled(!running);
}

void TreeExplorerPanel::clearSearch() {
    search_->blockSignals(true);
    search_->clear();
    search_->blockSignals(false);
}

} // namespace bonsaiviewer::modules::tree_explorer
