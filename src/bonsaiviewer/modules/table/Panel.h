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

#ifndef IFCINTERFACE_MODULES_TABLE_PANEL_H
#define IFCINTERFACE_MODULES_TABLE_PANEL_H

#include "../../components/Panel.h"

class QLabel;
class QTableView;
class QToolButton;

namespace bonsaiviewer::modules::table {

// TI07 — the Table panel (bottom of the window): a toolbar (new table,
// modify, add column, refresh, export CSV), the table name and row count,
// and the table view. The panel is only the shell; TablePanelView drives it.
class TablePanel : public components::Panel {
    Q_OBJECT

public:
    explicit TablePanel(QWidget* parent = nullptr);

    QTableView* tableView() const { return table_view_; }
    void setSummary(const QString& text);
    // The table actions need a table; "New table" is always available.
    void setTableActionsEnabled(bool enabled);

signals:
    void newTableRequested();
    void modifyTableRequested();
    void addColumnRequested();
    void refreshRequested();
    void exportRequested();

private:
    QLabel* summary_label_ = nullptr;
    QTableView* table_view_ = nullptr;
    QToolButton* modify_button_ = nullptr;
    QToolButton* column_button_ = nullptr;
    QToolButton* refresh_button_ = nullptr;
    QToolButton* export_button_ = nullptr;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_PANEL_H
