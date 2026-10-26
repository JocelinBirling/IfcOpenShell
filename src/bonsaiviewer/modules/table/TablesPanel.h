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

#ifndef IFCINTERFACE_MODULES_TABLE_TABLESPANEL_H
#define IFCINTERFACE_MODULES_TABLE_TABLESPANEL_H

#include "../../components/Panel.h"

class QToolButton;
class QTreeWidget;

namespace bonsaiviewer::modules::table {

// TI07 phase 2 — the Tables panel: a toolbar (new table, add folder, run,
// import) above the tree of saved folders and tables. The tree itself is
// populated by TablesView; the panel is only the shell.
class TablesPanel : public components::Panel {
    Q_OBJECT

public:
    explicit TablesPanel(QWidget* parent = nullptr);

    QTreeWidget* treeWidget() const { return tree_; }

    // The key (Qt::UserRole of column 0) of the selected node, or empty.
    QString currentKey() const;

    // Run follows the selection: only a saved table can be run.
    void setRunEnabled(bool enabled);

signals:
    void newTableRequested();
    void addFolderRequested();
    void runRequested();
    void importRequested();

private:
    QTreeWidget* tree_ = nullptr;
    QToolButton* run_button_ = nullptr;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_TABLESPANEL_H
