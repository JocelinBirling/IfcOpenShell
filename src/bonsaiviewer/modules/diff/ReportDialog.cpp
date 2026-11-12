// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
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

#include "ReportDialog.h"

#include "../../ports/ifcopenshell_util/Attr.h"
#include "../../ports/ifctester/PyValue.h"
#include "../../services/PopupService.h"

#include "element.h" // helpers: get_scalar_attributes
#include "pset.h"    // helpers: get_psets

#include <QBrush>
#include <QColor>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QSaveFile>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <QVBoxLayout>

#include <QFont>

#include <algorithm>
#include <map>

namespace bonsaiviewer::modules::diff {

namespace {

constexpr int kRowRole = Qt::UserRole + 1;

const QStringList& reportHeaders() {
    static const QStringList headers{"Model", "GlobalId", "Class", "Name", "Changes", "Pset / Group",
                                     "Property", "Old value", "New value", "Change kind"};
    return headers;
}

QString csvField(QString text, QChar separator) {
    if (text.contains(separator) || text.contains('"') || text.contains('\n')) {
        text = '"' + text.replace("\"", "\"\"") + '"';
    }
    return text;
}

// The colours of spec 34 (Properties panel): added, removed, changed.
const QColor kAdded("#3CB44B");
const QColor kRemoved("#E6194B");
const QColor kChanged("#F08A24");

} // namespace

// ---------------------------------------------------------------------------
// ReportDialog

ReportDialog::ReportDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("IfcDiff Report");
    setModal(false);
    resize(1100, 560);
    auto* layout = new QVBoxLayout(this);

    auto* top = new QHBoxLayout();
    filter_ = new QLineEdit(this);
    filter_->setPlaceholderText("Filter the rows…");
    filter_->setClearButtonEnabled(true);
    auto* export_button = new QPushButton("Export CSV…", this);
    top->addWidget(filter_, 1);
    top->addWidget(export_button);
    layout->addLayout(top);

    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels(reportHeaders());
    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSourceModel(model_);
    proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_->setFilterKeyColumn(-1);
    view_ = new QTableView(this);
    view_->setModel(proxy_);
    view_->setSortingEnabled(true);
    view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view_->setContextMenuPolicy(Qt::CustomContextMenu);
    view_->horizontalHeader()->setStretchLastSection(true);
    view_->verticalHeader()->setVisible(false);
    layout->addWidget(view_, 1);

    auto* hint = new QLabel("Click: select in both models · Double click: isolate · Right click: compare Before / After", this);
    hint->setProperty("textRole", "secondary");
    layout->addWidget(hint);

    connect(filter_, &QLineEdit::textChanged, proxy_, &QSortFilterProxyModel::setFilterFixedString);
    connect(export_button, &QPushButton::clicked, this, &ReportDialog::exportCsv);
    connect(view_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        std::vector<std::uint32_t> ids;
        for (const QModelIndex& index : view_->selectionModel()->selectedRows()) {
            if (const ReportRow* row = rowAt(index.row())) ids.insert(ids.end(), row->object_ids.begin(), row->object_ids.end());
        }
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
        if (!ids.empty()) emit selectRequested(ids);
    });
    connect(view_, &QTableView::doubleClicked, this, [this](const QModelIndex& index) {
        if (const ReportRow* row = rowAt(index.row())) emit isolateRequested(row->object_ids);
    });
    connect(view_, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        const ReportRow* row = rowAt(view_->indexAt(pos).row());
        if (row == nullptr) return;
        const QString guid = row->global_id;
        const std::vector<std::uint32_t> ids = row->object_ids;
        QMenu menu(this);
        menu.addAction("Isolate", this, [this, ids]() { emit isolateRequested(ids); });
        menu.addAction("Compare Before / After", this, [this, guid]() { emit compareRequested(guid); });
        menu.exec(view_->viewport()->mapToGlobal(pos));
    });
}

void ReportDialog::setRows(std::vector<ReportRow> rows) {
    rows_ = std::move(rows);
    view_->setSortingEnabled(false);
    model_->removeRows(0, model_->rowCount());
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const ReportRow& row = rows_[i];
        QList<QStandardItem*> items;
        for (const QString& text : {row.model, row.global_id, row.ifc_class, row.name, row.changes, row.group,
                                    row.property, row.old_value, row.new_value, row.kind}) {
            auto* item = new QStandardItem(text);
            item->setData(static_cast<int>(i), kRowRole);
            items.append(item);
        }
        QColor colour;
        if (row.changes == "added" || row.kind.startsWith("property added") || row.kind.startsWith("value added")) colour = kAdded;
        else if (row.changes == "deleted" || row.kind.startsWith("property removed") || row.kind.startsWith("value removed")) colour = kRemoved;
        else if (!row.kind.isEmpty()) colour = kChanged;
        if (colour.isValid()) items.last()->setForeground(QBrush(colour));
        model_->appendRow(items);
    }
    view_->setSortingEnabled(true);
    view_->resizeColumnsToContents();
    setWindowTitle(QString("IfcDiff Report — %1 row(s)").arg(rows_.size()));
}

const ReportRow* ReportDialog::rowAt(int displayed_row) const {
    if (displayed_row < 0) return nullptr;
    const QModelIndex index = proxy_->index(displayed_row, 0);
    bool ok = false;
    const int row = index.data(kRowRole).toInt(&ok);
    if (!ok || row < 0 || static_cast<std::size_t>(row) >= rows_.size()) return nullptr;
    return &rows_[static_cast<std::size_t>(row)];
}

void ReportDialog::exportCsv() {
    const QString path = QFileDialog::getSaveFileName(this, "Export the Report", "IfcDiff report.csv", "CSV files (*.csv)");
    if (path.isEmpty()) return;
    // As the Table panel: "," decimals go with ";" separators.
    const QChar separator = QLocale().decimalPoint().at(0) == ',' ? ';' : ',';
    QStringList lines;
    QStringList header;
    for (const QString& title : reportHeaders()) header.append(csvField(title, separator));
    lines.append(header.join(separator));
    for (int displayed = 0; displayed < proxy_->rowCount(); ++displayed) {
        QStringList fields;
        for (int column = 0; column < proxy_->columnCount(); ++column) {
            fields.append(csvField(proxy_->index(displayed, column).data().toString(), separator));
        }
        lines.append(fields.join(separator));
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        services::popup::information(this, QString("Cannot write %1: %2").arg(path, file.errorString()));
        return;
    }
    file.write("\xEF\xBB\xBF"); // UTF-8 BOM, for spreadsheet applications
    file.write((lines.join("\r\n") + "\r\n").toUtf8());
    if (!file.commit()) services::popup::information(this, QString("Cannot write %1: %2").arg(path, file.errorString()));
}

// ---------------------------------------------------------------------------
// CompareDialog

namespace {

// The fields of one element, in display order: "Pset / Property" -> value.
struct Fields {
    std::vector<QString> order;
    std::map<QString, QString> values;

    void add(const QString& key, const QString& value) {
        if (!values.count(key)) order.push_back(key);
        values[key] = value;
    }
};

Fields fieldsOf(const QString& model, const express::base& element) {
    namespace util = bonsaiviewer::ports::ifcopenshell_util;
    Fields fields;
    if (!element) return fields;
    fields.add("Model", model);
    fields.add("GlobalId", QString::fromStdString(util::forwardString(element, "GlobalId").value_or("")));
    fields.add("Class", QString::fromStdString(util::className(element)));
    for (const auto& [name, value] : get_scalar_attributes(element)) {
        if (name == "GlobalId") continue;
        fields.add(QString::fromStdString(name), QString::fromStdString(value));
    }
    try {
        for (const auto& [pset, properties] : get_psets(element)) {
            for (const auto& [name, value] : properties) {
                if (name == "id") continue;
                fields.add(QString::fromStdString(pset + " / " + name),
                           value.is_null() ? QString() : QString::fromStdString(bonsaiviewer::ports::ifctester::pyStr(value)));
            }
        }
    } catch (...) {
    }
    return fields;
}

} // namespace

CompareDialog::CompareDialog(const QString& before_model, const express::base& before, const QString& after_model,
                             const express::base& after, QWidget* parent)
    : QDialog(parent) {
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    resize(760, 620);
    const Fields a = fieldsOf(before_model, before);
    const Fields b = fieldsOf(after_model, after);
    const QString guid = !b.values.empty() ? b.values.at("GlobalId") : (!a.values.empty() ? a.values.at("GlobalId") : QString());
    setWindowTitle(QString("Compare — %1").arg(guid));

    // The union of the fields: the After order, then what only Before has.
    std::vector<QString> keys = b.order;
    for (const QString& key : a.order) {
        if (!b.values.count(key)) keys.push_back(key);
    }
    auto* layout = new QVBoxLayout(this);
    table_ = new QTableWidget(static_cast<int>(keys.size()), 2, this);
    table_->setHorizontalHeaderLabels({"Before", "After"});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for (int row = 0; row < static_cast<int>(keys.size()); ++row) {
        const QString& key = keys[static_cast<std::size_t>(row)];
        table_->setVerticalHeaderItem(row, new QTableWidgetItem(key));
        const auto in_a = a.values.find(key);
        const auto in_b = b.values.find(key);
        auto* left = new QTableWidgetItem(in_a == a.values.end() ? QString() : in_a->second);
        auto* right = new QTableWidgetItem(in_b == b.values.end() ? QString() : in_b->second);
        // Model is always different: not a change.
        if (key != "Model") {
            QColor colour;
            if (in_a == a.values.end() && before) colour = kAdded;
            else if (in_b == b.values.end() && after) colour = kRemoved;
            else if (in_a != a.values.end() && in_b != b.values.end() && in_a->second != in_b->second) colour = kChanged;
            if (colour.isValid()) {
                left->setForeground(QBrush(colour));
                right->setForeground(QBrush(colour));
                QFont font = right->font();
                font.setBold(true);
                left->setFont(font);
                right->setFont(font);
            }
        }
        table_->setItem(row, 0, left);
        table_->setItem(row, 1, right);
    }
    layout->addWidget(table_, 1);
    auto* legend = new QLabel(QString("<span style=\"color:%1\">■ added</span> &nbsp; <span style=\"color:%2\">■ removed</span>"
                                      " &nbsp; <span style=\"color:%3\">■ changed</span>")
                                  .arg(kAdded.name(), kRemoved.name(), kChanged.name()),
                              this);
    legend->setTextFormat(Qt::RichText);
    layout->addWidget(legend);
}

} // namespace bonsaiviewer::modules::diff
