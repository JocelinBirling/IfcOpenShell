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

#ifndef IFCINTERFACE_MODULES_TABLE_TABLEMODEL_H
#define IFCINTERFACE_MODULES_TABLE_TABLEMODEL_H

#include "Table.h"

#include <QAbstractTableModel>
#include <QColor>

#include <cstdint>
#include <functional>
#include <vector>

namespace bonsaiviewer::services { class IfcDataAccess; }

namespace bonsaiviewer::modules::table {

// TI07 — the rows and columns of a table. A row is a group of elements: one
// element, or several when the table is grouped (the elements that share the
// values of the group-key columns). A row's values are read through B05 the
// first time the row is shown, then cached.
class TableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    // The display name of a model, by session model id.
    using ModelNameProvider = std::function<QString(std::uint32_t)>;

    TableModel(bonsaiviewer::services::IfcDataAccess& data_access,
               ModelNameProvider model_name,
               QObject* parent = nullptr);

    void setColumns(const std::vector<TableColumn>& columns);
    // The elements of the table; they are grouped when a column is a group key.
    void setRows(const std::vector<std::uint32_t>& object_ids);
    // One colour per row, for the Color column (empty = no colours).
    void setRowColours(const std::vector<QColor>& colours);

    // The elements of a row (one or more).
    const std::vector<std::uint32_t>& objects(int row) const { return groups_.at(static_cast<size_t>(row)); }
    // Every element of the table, in order.
    const std::vector<std::uint32_t>& elements() const { return rows_; }

    // The value of a cell: a double, a qlonglong or a QString (empty when
    // the element has no such value). "ERROR" when an aggregation cannot apply.
    QVariant value(int row, int column) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // "12.5" / "12,5" -> 12.5; anything else unchanged.
    static QVariant autoNumeric(const QString& text);

private:
    void regroup();
    std::vector<QVariant> elementValues(std::uint32_t object_id) const;
    const std::vector<QVariant>& rowValues(int row) const;
    // Combines the values of one column over the elements of a group.
    static QVariant aggregate(const std::vector<QVariant>& values, const TableColumn& column);

    bonsaiviewer::services::IfcDataAccess& data_access_;
    ModelNameProvider model_name_;
    std::vector<TableColumn> columns_;
    std::vector<std::uint32_t> rows_;                 // the elements, in order
    std::vector<std::vector<std::uint32_t>> groups_;  // one entry per row
    std::vector<QColor> row_colours_;
    mutable std::vector<std::vector<QVariant>> cache_; // empty = not read yet
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_TABLEMODEL_H
