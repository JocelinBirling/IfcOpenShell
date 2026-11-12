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

#include "View.h"

#include "Panel.h"
#include "Sources.h"

#include "../../services/TreePanelFramework.h"

#include <QComboBox>
#include <QSettings>
#include <QTreeWidget>

namespace bonsaiviewer::modules::tree_explorer {

namespace {

QString settingsKey(const TreeExplorerPanel* panel) {
    return QStringLiteral("treeExplorer/") + panel->objectName();
}

} // namespace

TreeExplorerView::TreeExplorerView(TreeExplorerPanel* panel, const Context& context, QObject* parent)
    : QObject(parent), panel_(panel), context_(context)
{
    QComboBox* selector = panel_->treeSelector();
    for (const TreeKind& kind : treeKinds()) selector->addItem(kind.title, kind.id);

    connect(panel_, &TreeExplorerPanel::treeChosen, this, &TreeExplorerView::setTree);
    connect(panel_, &TreeExplorerPanel::activeModelOnlyChanged, this, [this](bool active_only) {
        active_only_ = active_only;
        if (source_) source_->setActiveModelOnly(active_only);
    });
    connect(panel_, &TreeExplorerPanel::computeSettingsRequested, this, [this]() {
        if (source_) source_->openComputeSettings();
    });
    connect(panel_, &TreeExplorerPanel::computeRunRequested, this, [this]() {
        if (source_) source_->runCompute();
    });
    connect(panel_, &TreeExplorerPanel::searchChanged, this, [this](const QString& text) {
        filter_ = text;
        if (tree_panel_) tree_panel_->forgetExpandedState();
        if (source_) source_->setFilter(text);
    });

    // The tree of this panel last time, else the last one chosen in any panel.
    QSettings settings;
    QString tree_id = settings.value(settingsKey(panel_)).toString();
    if (tree_id.isEmpty()) tree_id = settings.value(QStringLiteral("treeExplorer/last")).toString();
    if (selector->findData(tree_id) < 0) tree_id = selector->itemData(0).toString();
    setTree(tree_id);
}

void TreeExplorerView::setTree(const QString& tree_id) {
    if (tree_id == tree_id_ && source_) return;
    IndexedSource* source = makeSource(tree_id, context_, this);
    if (source == nullptr) return;

    delete tree_panel_;
    delete source_;
    panel_->tree()->clear();
    tree_id_ = tree_id;
    source_ = source;

    QComboBox* selector = panel_->treeSelector();
    selector->setCurrentIndex(selector->findData(tree_id));
    panel_->setScopeEnabled(source_->supportsScope());
    panel_->setInterpreted(source_->interpreted());
    panel_->setComputeRowVisible(source_->hasCompute());
    const auto showComputeStatus = [this]() {
        panel_->setComputeStatus(source_->computeStatus());
        panel_->setComputeRunning(source_->isComputing());
    };
    showComputeStatus();
    connect(source_, &IndexedSource::computeStatusChanged, this, showComputeStatus);

    source_->setActiveModelOnly(active_only_);
    source_->setFilter(filter_);
    tree_panel_ = new bonsaiviewer::services::tree::TreePanelWidget(panel_->tree(), source_, this);
    tree_panel_->setLazyChildren(true);
    tree_panel_->setEyeColumn(true);
    tree_panel_->setExpandMenu(true);
    // Several nodes at once (Ctrl / Shift); the viewport selection follows.
    // After the TreePanelWidget, which sets single selection.
    QTreeWidget* tree = panel_->tree();
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(tree, &QTreeWidget::itemSelectionChanged, source_, [this, tree]() {
        QStringList keys;
        for (QTreeWidgetItem* item : tree->selectedItems()) keys << item->data(0, Qt::UserRole).toString();
        source_->onSelectionChanged(keys);
    });
    source_->rebuild();

    QSettings settings;
    settings.setValue(settingsKey(panel_), tree_id);
    settings.setValue(QStringLiteral("treeExplorer/last"), tree_id);
}

} // namespace bonsaiviewer::modules::tree_explorer
