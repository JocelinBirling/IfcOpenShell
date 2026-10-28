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

#include "../../services/RelationValues.h"

#include "../../ElementRegistry.h"
#include "../../services/IfcDataAccess.h"

#include <QBrush>
#include <QHash>
#include <QStringList>

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

bool isEmptyValue(const QVariant& value) {
    return !value.isValid() || (value.typeId() == QMetaType::QString && value.toString().isEmpty());
}

QString joinTexts(const std::vector<QVariant>& values) {
    QStringList texts;
    for (const QVariant& value : values) texts << value.toString();
    return texts.join("; ");
}

} // namespace

TableModel::TableModel(services::IfcDataAccess& data_access, ModelNameProvider model_name, QObject* parent)
    : QAbstractTableModel(parent), data_access_(data_access), model_name_(std::move(model_name)) {}

void TableModel::setColumns(const std::vector<TableColumn>& columns) {
    beginResetModel();
    columns_ = columns;
    regroup();
    endResetModel();
}

void TableModel::setRows(const std::vector<std::uint32_t>& object_ids) {
    beginResetModel();
    rows_ = object_ids;
    row_colours_.clear();
    regroup();
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

// One row per distinct value of the group-key columns, in first-seen order.
// Without group-key columns, one row per element.
void TableModel::regroup() {
    groups_.clear();
    std::vector<size_t> key_columns;
    for (size_t column = 0; column < columns_.size(); ++column) {
        if (columns_[column].group_key) key_columns.push_back(column);
    }

    if (key_columns.empty()) {
        groups_.reserve(rows_.size());
        for (const std::uint32_t object_id : rows_) groups_.push_back({object_id});
    } else {
        QHash<QString, size_t> row_of_key;
        for (const std::uint32_t object_id : rows_) {
            const std::vector<QVariant> values = elementValues(object_id);
            QString key;
            for (const size_t column : key_columns) {
                key += values[column].toString();
                key += QChar(0x1F);
            }
            const auto found = row_of_key.constFind(key);
            if (found == row_of_key.constEnd()) {
                row_of_key.insert(key, groups_.size());
                groups_.push_back({object_id});
            } else {
                groups_[found.value()].push_back(object_id);
            }
        }
    }
    cache_.assign(groups_.size(), {});
}

// The values of one element, one per column (Count is 1 per element).
std::vector<QVariant> TableModel::elementValues(std::uint32_t object_id) const {
    std::vector<QVariant> values;
    values.reserve(columns_.size());
    const auto info = data_access_.basicInfo(object_id);
    bool sets_read = false;
    element_properties psets;
    element_properties qtos;
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
            values.emplace_back(qlonglong(1));
            break;
        case TableColumn::Kind::Color:
            values.emplace_back(QString());
            break;
        case TableColumn::Kind::Relation: {
            const auto entity = data_access_.entity(object_id);
            const QString model = info && model_name_ ? model_name_(info->session_model_id) : QString();
            values.emplace_back(entity ? services::relations::value(*entity, column.property, model)
                                       : (column.property == QLatin1String("model") ? model : QString()));
            break;
        }
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

// Combines the values of one column over the elements of a group.
QVariant TableModel::aggregate(const std::vector<QVariant>& values, const TableColumn& column) {
    if (column.group_key) return values.empty() ? QVariant(QString()) : values.front();

    std::vector<QVariant> present;
    for (const QVariant& value : values) {
        if (!isEmptyValue(value)) present.push_back(value);
    }

    using Aggregation = TableColumn::Aggregation;
    switch (column.aggregation) {
    case Aggregation::Count:
        return qlonglong(present.size());
    case Aggregation::Concatenate:
        return joinTexts(present);
    case Aggregation::Default:
        break;
    default:
        break;
    }

    if (present.empty()) return QString();

    if (column.aggregation == Aggregation::Default) {
        std::vector<QVariant> numbers;
        std::vector<QVariant> texts;
        for (const QVariant& value : present) (isNumber(value) ? numbers : texts).push_back(value);
        if (texts.empty()) {
            double sum = 0.0;
            for (const QVariant& value : numbers) sum += value.toDouble();
            return sum;
        }
        if (numbers.empty()) return joinTexts(texts);
        double sum = 0.0;
        for (const QVariant& value : numbers) sum += value.toDouble();
        return QString::number(sum) + "; " + joinTexts(texts);
    }

    // Sum, Average, Min and Max apply to numbers only.
    for (const QVariant& value : present) {
        if (!isNumber(value)) return QString("ERROR");
    }
    double sum = 0.0;
    double low = present.front().toDouble();
    double high = low;
    for (const QVariant& value : present) {
        const double number = value.toDouble();
        sum += number;
        low = qMin(low, number);
        high = qMax(high, number);
    }
    switch (column.aggregation) {
    case Aggregation::Average: return sum / static_cast<double>(present.size());
    case Aggregation::Min: return low;
    case Aggregation::Max: return high;
    default: return sum;
    }
}

const std::vector<QVariant>& TableModel::rowValues(int row) const {
    std::vector<QVariant>& values = cache_[static_cast<size_t>(row)];
    if (!values.empty() || columns_.empty()) return values;

    const std::vector<std::uint32_t>& group = groups_[static_cast<size_t>(row)];
    std::vector<std::vector<QVariant>> elements;
    elements.reserve(group.size());
    for (const std::uint32_t object_id : group) elements.push_back(elementValues(object_id));

    values.reserve(columns_.size());
    for (size_t column = 0; column < columns_.size(); ++column) {
        const TableColumn& definition = columns_[column];
        if (definition.kind == TableColumn::Kind::Count) {
            values.emplace_back(qlonglong(group.size()));
            continue;
        }
        if (definition.kind == TableColumn::Kind::Color) {
            values.emplace_back(QString());
            continue;
        }
        std::vector<QVariant> column_values;
        column_values.reserve(elements.size());
        for (const auto& element : elements) column_values.push_back(element[column]);
        values.push_back(aggregate(column_values, definition));
    }
    return values;
}

QVariant TableModel::value(int row, int column) const {
    if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount()) return {};
    return rowValues(row)[static_cast<size_t>(column)];
}

int TableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(groups_.size());
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
