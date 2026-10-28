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

#ifndef IFCINTERFACE_MODULES_TABLE_TABLEPROXY_H
#define IFCINTERFACE_MODULES_TABLE_TABLEPROXY_H

#include <QSet>
#include <QSortFilterProxyModel>
#include <QStringList>

#include <map>

namespace bonsaiviewer::modules::table {

class TableModel;

// BonsaiViewer07 phase 3 — the rows as displayed: sorted on one column and
// filtered on any number of columns. A column filter keeps the rows whose
// cell text is one of the chosen values. Numbers sort as numbers, texts
// alphabetically (case-insensitive), empty cells last.
class TableProxy : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit TableProxy(TableModel* model, QObject* parent = nullptr);

    // The text a cell is filtered on ("" for an empty cell).
    QString cellText(int source_row, int column) const;
    // The distinct texts of a column over every row of the table, sorted.
    QStringList columnValues(int column) const;

    // Keeps the rows whose cell text is in `kept`.
    void setColumnFilter(int column, const QSet<QString>& kept);
    void clearColumnFilter(int column);
    void clearFilters();
    bool isFiltered(int column) const { return filters_.count(column) > 0; }
    // The kept values of a filtered column.
    QSet<QString> columnFilter(int column) const;

    // The source row of a displayed row (-1 when out of range).
    int sourceRow(int displayed_row) const;

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    TableModel* model_ = nullptr;
    std::map<int, QSet<QString>> filters_;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_TABLEPROXY_H
