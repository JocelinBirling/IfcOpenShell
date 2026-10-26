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

#include "TableModel.h"

#include "../../ElementRegistry.h"
#include "../../services/IfcDataAccess.h"

#include <QBrush>

#include <sstream>

namespace bonsaiviewer::modules::table {

namespace {

QVariant propertyVariant(const property_value& value, bool auto_numeric) {
    if (value.is_null()) return QString();
    if (const auto* number = value.get_if<double>()) return *number;
    if (const auto* number = value.get_if<std::int64_t>()) return static_cast<qlonglong>(*number);
    if (const auto* flag = value.get_if<bool>()) return QString(*flag ? "True" : "False");
    QString text;
    if (const auto* string = value.get_if<std::string>()) {
        text = QString::fromStdString(*string);
    } else {
        std::ostringstream stream;
        stream << value;
        text = QString::fromStdString(stream.str());
    }
    return auto_numeric ? TableModel::autoNumeric(text) : QVariant(text);
}

QVariant findProperty(const element_properties& sets, const TableColumn& column) {
    const auto set = sets.find(column.pset.toStdString());
    if (set == sets.end()) return {};
    const auto property = set->second.find(column.property.toStdString());
    if (property == set->second.end()) return {};
    return propertyVariant(property->second, column.auto_numeric);
}

bool isNumber(const QVariant& value) {
    return value.typeId() == QMetaType::Double || value.typeId() == QMetaType::LongLong;
}

} // namespace

TableModel::TableModel(services::IfcDataAccess& data_access, ModelNameProvider model_name, QObject* parent)
    : QAbstractTableModel(parent), data_access_(data_access), model_name_(std::move(model_name)) {}

void TableModel::setColumns(const std::vector<TableColumn>& columns) {
    beginResetModel();
    columns_ = columns;
    cache_.assign(rows_.size(), {});
    endResetModel();
}

void TableModel::setRows(const std::vector<std::uint32_t>& object_ids) {
    beginResetModel();
    rows_ = object_ids;
    row_colours_.clear();
    cache_.assign(rows_.size(), {});
    endResetModel();
}

void TableModel::setRowColours(const std::vector<QColor>& colours) {
    row_colours_ = colours;
    if (!colours.empty() && !columns_.empty()) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), {Qt::BackgroundRole});
    }
}

QVariant TableModel::autoNumeric(const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || (trimmed.contains('.') && trimmed.contains(','))) return text;
    bool ok = false;
    const double number = QString(trimmed).replace(',', '.').toDouble(&ok);
    return ok ? QVariant(number) : QVariant(text);
}

const std::vector<QVariant>& TableModel::rowValues(int row) const {
    std::vector<QVariant>& values = cache_[static_cast<size_t>(row)];
    if (!values.empty() || columns_.empty()) return values;

    const uint32_t object_id = rows_[static_cast<size_t>(row)];
    const auto info = data_access_.basicInfo(object_id);
    bool sets_read = false;
    element_properties psets;
    element_properties qtos;
    values.reserve(columns_.size());
    for (const TableColumn& column : columns_) {
        switch (column.kind) {
        case TableColumn::Kind::Name:
            values.emplace_back(info ? info->name : QString());
            break;
        case TableColumn::Kind::Class:
            values.emplace_back(info ? info->type : QString());
            break;
        case TableColumn::Kind::GlobalId:
            values.emplace_back(info ? info->guid : QString());
            break;
        case TableColumn::Kind::Model:
            values.emplace_back(info && model_name_ ? model_name_(info->session_model_id) : QString());
            break;
        case TableColumn::Kind::Count:
            values.emplace_back(qlonglong(1)); // one element per row until grouping
            break;
        case TableColumn::Kind::Color:
            values.emplace_back(QString()); // the colour is the cell background
            break;
        case TableColumn::Kind::Property: {
            if (!sets_read) {
                psets = data_access_.propertySets(object_id);
                qtos = data_access_.quantitySets(object_id);
                sets_read = true;
            }
            QVariant value = findProperty(psets, column);
            if (!value.isValid()) value = findProperty(qtos, column);
            values.push_back(value.isValid() ? value : QVariant(QString()));
            break;
        }
        }
    }
    return values;
}

QVariant TableModel::value(int row, int column) const {
    if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount()) return {};
    return rowValues(row)[static_cast<size_t>(column)];
}

int TableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

int TableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(columns_.size());
}

QVariant TableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    if (role == Qt::DisplayRole || role == Qt::EditRole) return value(index.row(), index.column());
    if (role == Qt::BackgroundRole && columns_[static_cast<size_t>(index.column())].kind == TableColumn::Kind::Color
        && static_cast<size_t>(index.row()) < row_colours_.size()) {
        return QBrush(row_colours_[static_cast<size_t>(index.row())]);
    }
    if (role == Qt::TextAlignmentRole) {
        const bool number = isNumber(value(index.row(), index.column()));
        return QVariant::fromValue((number ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
    }
    return {};
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && section >= 0 && section < columnCount()) {
        const TableColumn& column = columns_[static_cast<size_t>(section)];
        if (role == Qt::DisplayRole) return column.displayTitle();
        if (role == Qt::ToolTipRole && column.kind == TableColumn::Kind::Property) {
            return QString("%1 . %2").arg(column.pset, column.property);
        }
        return {};
    }
    if (orientation == Qt::Vertical && role == Qt::DisplayRole) return section + 1;
    return {};
}

} // namespace bonsaiviewer::modules::table
