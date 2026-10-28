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

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::unified_tree {

UnifiedTreePanel::UnifiedTreePanel(QWidget* parent)
    : components::Panel("USpatial Tree", nullptr, parent, /*has_settings=*/true)
{
    auto* section = new components::Section("", components::SectionHeaderMode::Hidden, this);
    section->setBodyExpanding(true);

    auto* body = new QWidget(section);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* banner = new QLabel("Unified Spatial Tree is calculated from the models, not directly in it.", body);
    banner->setObjectName("unifiedTreeBanner");
    banner->setWordWrap(true);
    banner->setStyleSheet("QLabel#unifiedTreeBanner { background: rgba(255, 190, 60, 60);"
                          " border: 1px solid rgba(255, 190, 60, 160); border-radius: 4px; padding: 4px 6px; }");
    layout->addWidget(banner);

    auto* row = new QWidget(body);
    auto* row_layout = new QHBoxLayout(row);
    row_layout->setContentsMargins(0, 0, 0, 0);
    run_ = new QPushButton("Run", row);
    run_->setToolTip("Compute the Unified Tree with the current settings");
    status_ = new QLabel("Not computed. Choose the settings, then Run.", row);
    status_->setProperty("textRole", "secondary");
    status_->setWordWrap(true);
    row_layout->addWidget(run_);
    row_layout->addWidget(status_, 1);
    layout->addWidget(row);

    tree_ = new QTreeWidget(body);
    tree_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tree_->setColumnCount(2);
    tree_->setHeaderHidden(true);
    tree_->setIconSize(QSize(16, 16));
    tree_->setUniformRowHeights(true);
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    // Name stretches; the second column holds the "deduced" icon.
    tree_->header()->setStretchLastSection(false);
    tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    tree_->header()->resizeSection(1, 24);
    layout->addWidget(tree_, 1);

    section->addBodyWidget(body);
    addBodyWidget(section);

    connect(run_, &QPushButton::clicked, this, &UnifiedTreePanel::runRequested);
}

void UnifiedTreePanel::setStatus(const QString& text) {
    status_->setText(text);
}

void UnifiedTreePanel::setRunning(bool running) {
    run_->setEnabled(!running);
}

} // namespace bonsaiviewer::modules::unified_tree
