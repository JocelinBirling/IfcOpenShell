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

#include "TablesView.h"

#include "TableStore.h"
#include "TablesPanel.h"

#include "../../services/ContextMenuBuilder.h"
#include "../../services/PopupService.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QTreeWidget>

namespace bonsaiviewer::modules::table {

using bonsaiviewer::services::ContextMenuBuilder;
using bonsaiviewer::services::tree::TreeNode;

namespace {

TreeNode folderNode(const QString& id, const QString& name) {
    TreeNode node;
    node.key = id;
    node.label = name;
    node.icon = ":/icons/folder.svg";
    node.is_folder = true;
    node.accepts_children = true;
    return node;
}

TreeNode tableNode(const QString& id, const QString& name) {
    TreeNode node;
    node.key = id;
    node.label = name;
    return node;
}

} // namespace

TablesView::TablesView(TablesPanel* panel, TableStore& store, QObject* parent)
    : TreeDataSource(parent),
      panel_(panel),
      store_(store)
{
    tree_panel_ = new bonsaiviewer::services::tree::TreePanelWidget(panel_->treeWidget(), this, this);
    connect(&store_, &TableStore::changed, this, &TablesView::nodesChanged);

    connect(panel_, &TablesPanel::newTableRequested, this,
            [this] { emit newTableRequested(targetFolder()); });
    connect(panel_, &TablesPanel::addFolderRequested, this, [this] { addFolder(targetFolder()); });
    connect(panel_, &TablesPanel::importRequested, this, [this] { importTable(targetFolder()); });
    connect(panel_, &TablesPanel::runRequested, this, [this] {
        const QString key = panel_->currentKey();
        if (store_.table(key)) emit openRequested(key);
    });
    connect(panel_->treeWidget(), &QTreeWidget::currentItemChanged, this, [this] {
        panel_->setRunEnabled(store_.table(panel_->currentKey()).has_value());
    });

    emit nodesChanged();
}

std::vector<TreeNode> TablesView::topLevelNodes() {
    std::vector<TreeNode> nodes;
    for (const auto& entry : store_.folders()) {
        if (entry.parent_id.isEmpty()) nodes.push_back(folderNode(entry.id, entry.name));
    }
    for (const auto& document : store_.tablesInFolder({})) {
        nodes.push_back(tableNode(document.id, document.definition.name));
    }
    return nodes;
}

std::vector<TreeNode> TablesView::childrenOf(const QString& key) {
    std::vector<TreeNode> nodes;
    for (const auto& entry : store_.folders()) {
        if (entry.parent_id == key) nodes.push_back(folderNode(entry.id, entry.name));
    }
    for (const auto& document : store_.tablesInFolder(key)) {
        nodes.push_back(tableNode(document.id, document.definition.name));
    }
    return nodes;
}

void TablesView::populateContextMenu(const QString& key, ContextMenuBuilder& menu) {
    if (key.isEmpty()) {
        menu.addItem({.id = "new-table", .label = "New Table…",
                      .trigger = [this]() { emit newTableRequested(QString()); }});
        menu.addItem({.id = "add-folder", .label = "Add Folder…",
                      .trigger = [this]() { addFolder(QString()); }});
        menu.addSeparator();
        menu.addItem({.id = "import", .label = "Import Table…",
                      .trigger = [this]() { importTable(QString()); }});
        return;
    }
    if (store_.isFolder(key)) {
        menu.addItem({.id = "new-table", .label = "New Table…",
                      .trigger = [this, key]() { emit newTableRequested(key); }});
        menu.addItem({.id = "add-folder", .label = "Add Folder…",
                      .trigger = [this, key]() { addFolder(key); }});
        menu.addSeparator();
        menu.addItem({.id = "rename", .label = "Rename…",
                      .trigger = [this, key]() { renameFolder(key); }});
        menu.addItem({.id = "delete", .label = "Delete",
                      .trigger = [this, key]() { deleteFolder(key); }});
        menu.addSeparator();
        menu.addItem({.id = "import", .label = "Import Table Here…",
                      .trigger = [this, key]() { importTable(key); }});
        return;
    }
    // A table node.
    menu.addItem({.id = "run", .label = "Run",
                  .trigger = [this, key]() { emit openRequested(key); }});
    menu.addSeparator();
    menu.addItem({.id = "export", .label = "Export…",
                  .trigger = [this, key]() { exportTable(key); }});
    menu.addSeparator();
    menu.addItem({.id = "delete", .label = "Delete",
                  .trigger = [this, key]() { deleteTable(key); }});
}

void TablesView::onVisibilityToggled(const QString&, bool) {
    // The tables tree has no visibility column.
}

void TablesView::onNodeDoubleClicked(const QString& key) {
    if (store_.table(key)) emit openRequested(key);
}

void TablesView::onNodeMoved(const QString& key, const QString& new_parent) {
    // Only folders accept drops, so a non-folder parent can only come from a
    // stale tree: ignore it.
    if (!new_parent.isEmpty() && !store_.isFolder(new_parent)) return;
    store_.moveNode(key, new_parent);
}

QString TablesView::targetFolder() const {
    const QString key = panel_->currentKey();
    if (key.isEmpty()) return {};
    if (store_.isFolder(key)) return key;
    const auto document = store_.table(key);
    return document ? document->folder_id : QString();
}

void TablesView::addFolder(const QString& parent_id) {
    const QString name = QInputDialog::getText(panel_, "New Folder", "Folder name");
    if (!name.trimmed().isEmpty()) {
        store_.addFolder(name.trimmed(), parent_id);
    }
}

void TablesView::renameFolder(const QString& folder_id) {
    const auto folder = store_.folder(folder_id);
    if (!folder) return;
    const QString name = QInputDialog::getText(panel_, "Rename Folder", "Folder name", QLineEdit::Normal,
                                               folder->name);
    if (!name.trimmed().isEmpty()) {
        store_.renameFolder(folder_id, name.trimmed());
    }
}

void TablesView::deleteFolder(const QString& folder_id) {
    const auto folder = store_.folder(folder_id);
    if (!folder) return;
    const auto answer = QMessageBox::question(
        panel_, "Delete Folder",
        QString("Delete the folder “%1” and everything in it?").arg(folder->name));
    if (answer == QMessageBox::Yes) store_.deleteFolder(folder_id);
}

void TablesView::deleteTable(const QString& table_id) {
    const auto document = store_.table(table_id);
    if (!document) return;
    const auto answer = QMessageBox::question(
        panel_, "Delete Table", QString("Delete the table “%1”?").arg(document->definition.name));
    if (answer == QMessageBox::Yes) store_.deleteTable(table_id);
}

void TablesView::exportTable(const QString& table_id) {
    const auto document = store_.table(table_id);
    if (!document) return;
    const QString path = QFileDialog::getSaveFileName(panel_, "Export the Table",
                                                      document->definition.name + ".json",
                                                      "Table files (*.json)");
    if (path.isEmpty()) return;
    if (!store_.exportTable(table_id, path)) {
        services::popup::information(panel_, QString("Cannot write %1").arg(QFileInfo(path).fileName()));
    }
}

void TablesView::importTable(const QString& folder_id) {
    const QString path = QFileDialog::getOpenFileName(panel_, "Import a Table", {}, "Table files (*.json)");
    if (path.isEmpty()) return;
    QString error;
    if (store_.importTable(path, folder_id, error).isEmpty()) {
        services::popup::information(panel_, QString("The table could not be imported: %1").arg(error));
    }
}

} // namespace bonsaiviewer::modules::table
