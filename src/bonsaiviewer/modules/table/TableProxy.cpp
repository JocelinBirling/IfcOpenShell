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

#include "TableProxy.h"

#include "TableModel.h"

#include <algorithm>

namespace bonsaiviewer::modules::table {

namespace {

// Alphabetical, case-insensitive; ties broken case-sensitively.
bool textLess(const QString& a, const QString& b) {
    const int order = QString::localeAwareCompare(a.toLower(), b.toLower());
    return order != 0 ? order < 0 : a < b;
}

bool isNumber(const QVariant& value) {
    const int type = value.typeId();
    return type == QMetaType::Double || type == QMetaType::LongLong || type == QMetaType::Int;
}

} // namespace

TableProxy::TableProxy(TableModel* model, QObject* parent)
    : QSortFilterProxyModel(parent), model_(model) {
    setSourceModel(model);
    setDynamicSortFilter(true);
}

QString TableProxy::cellText(int source_row, int column) const {
    return model_->value(source_row, column).toString();
}

QStringList TableProxy::columnValues(int column) const {
    QSet<QString> values;
    for (int row = 0; row < model_->rowCount(); ++row) values.insert(cellText(row, column));
    QStringList sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end(), [](const QString& a, const QString& b) {
        if (a.isEmpty() != b.isEmpty()) return b.isEmpty(); // empty last
        return textLess(a, b);
    });
    return sorted;
}

void TableProxy::setColumnFilter(int column, const QSet<QString>& kept) {
    filters_[column] = kept;
    invalidateRowsFilter();
}

void TableProxy::clearColumnFilter(int column) {
    if (filters_.erase(column) > 0) invalidateRowsFilter();
}

void TableProxy::clearFilters() {
    if (filters_.empty()) return;
    filters_.clear();
    invalidateRowsFilter();
}

QSet<QString> TableProxy::columnFilter(int column) const {
    const auto found = filters_.find(column);
    return found == filters_.end() ? QSet<QString>() : found->second;
}

int TableProxy::sourceRow(int displayed_row) const {
    if (displayed_row < 0 || displayed_row >= rowCount()) return -1;
    return mapToSource(index(displayed_row, 0)).row();
}

bool TableProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    const QVariant a = model_->value(left.row(), left.column());
    const QVariant b = model_->value(right.row(), right.column());
    const QString a_text = a.toString();
    const QString b_text = b.toString();
    // Empty cells stay at the end, whatever the order.
    const bool descending = sortOrder() == Qt::DescendingOrder;
    if (a_text.isEmpty() != b_text.isEmpty()) return descending ? a_text.isEmpty() : b_text.isEmpty();
    // Numbers before texts; numbers by value, texts alphabetically.
    if (isNumber(a) != isNumber(b)) return isNumber(a);
    if (isNumber(a)) return a.toDouble() < b.toDouble();
    return textLess(a_text, b_text);
}

bool TableProxy::filterAcceptsRow(int source_row, const QModelIndex&) const {
    for (const auto& [column, kept] : filters_) {
        if (!kept.contains(cellText(source_row, column))) return false;
    }
    return true;
}

} // namespace bonsaiviewer::modules::table
