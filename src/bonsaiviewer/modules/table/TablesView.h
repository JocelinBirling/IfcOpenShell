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

#ifndef IFCINTERFACE_MODULES_TABLE_TABLESVIEW_H
#define IFCINTERFACE_MODULES_TABLE_TABLESVIEW_H

#include "../../services/TreePanelFramework.h"

#include <QString>

#include <vector>

namespace bonsaiviewer::modules::table {

class TablesPanel;
class TableStore;

// TI07 phase 2 — the Tables panel view: the B12 tree of saved folders and
// tables (create, rename, delete, drag and drop, import and export). Running
// a table is left to the table panel, through openRequested.
class TablesView : public bonsaiviewer::services::tree::TreeDataSource {
    Q_OBJECT

public:
    TablesView(TablesPanel* panel, TableStore& store, QObject* parent = nullptr);

    // tree::TreeDataSource
    std::vector<bonsaiviewer::services::tree::TreeNode> topLevelNodes() override;
    std::vector<bonsaiviewer::services::tree::TreeNode> childrenOf(const QString& key) override;
    void populateContextMenu(const QString& key,
                             bonsaiviewer::services::ContextMenuBuilder& menu) override;
    void onVisibilityToggled(const QString& key, bool visible) override;
    // Double-click on a table loads it.
    void onNodeDoubleClicked(const QString& key) override;
    // Drag and drop: the store owns the tree, the widget only mirrors it.
    void onNodeMoved(const QString& key, const QString& new_parent) override;

signals:
    // Load a saved table into the table panel; with on_box, only the
    // elements of the current box are kept (Run on Box).
    void openRequested(const QString& table_id, bool on_box);
    // Create a new table in a folder (empty = tree root).
    void newTableRequested(const QString& folder_id);

private:
    // The folder that new items go into: the selected folder, or the folder
    // of the selected table, or the root.
    QString targetFolder() const;
    void addFolder(const QString& parent_id);
    void renameFolder(const QString& folder_id);
    void deleteFolder(const QString& folder_id);
    void deleteTable(const QString& table_id);
    // A copy "<name> (copy)" in the same folder.
    void duplicateTable(const QString& table_id);
    void exportTable(const QString& table_id);
    void importTable(const QString& folder_id);

    TablesPanel* panel_ = nullptr;
    TableStore& store_;
    bonsaiviewer::services::tree::TreePanelWidget* tree_panel_ = nullptr;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_TABLESVIEW_H
