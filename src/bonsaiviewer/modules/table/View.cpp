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

#include "View.h"

#include "ColumnDialog.h"
#include "CreationDialog.h"
#include "Panel.h"
#include "TableModel.h"
#include "TableStore.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../services/ActionPipeline.h"
#include "../../services/PopupService.h"
#include "../../services/SelectionService.h"
#include "../../../ifcviewer/SceneLoader.h"

#include <QCursor>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLocale>
#include <QMenu>
#include <QPointer>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QTableView>
#include <QTimer>

#include <algorithm>
#include <set>

namespace bonsaiviewer::modules::table {

namespace {

std::vector<TableColumn> defaultColumns() {
    TableColumn name;
    name.kind = TableColumn::Kind::Name;
    TableColumn ifc_class;
    ifc_class.kind = TableColumn::Kind::Class;
    return {name, ifc_class};
}

QString csvField(const QVariant& value, QChar separator, QChar decimal_point) {
    QString text;
    if (value.typeId() == QMetaType::Double) {
        text = QString::number(value.toDouble(), 'g', 15).replace('.', decimal_point);
    } else {
        text = value.toString();
    }
    if (text.contains(separator) || text.contains('"') || text.contains('\n')) {
        text = '"' + text.replace("\"", "\"\"") + '"';
    }
    return text;
}

} // namespace

TablePanelView::TablePanelView(TablePanel* panel,
                               SessionState& session,
                               services::IdsBridge& bridge,
                               services::IfcDataAccess& data_access,
                               services::PropertyCatalog& catalog,
                               services::SelectionService& selection,
                               services::ActionPipeline& actions,
                               TableStore& store,
                               QObject* parent)
    : QObject(parent),
      panel_(panel),
      session_(session),
      bridge_(bridge),
      catalog_(catalog),
      selection_(selection),
      actions_(actions),
      store_(store)
{
    model_ = new TableModel(
        data_access,
        [this](std::uint32_t session_model_id) { return QFileInfo(ifcPath(session_model_id)).fileName(); },
        this);
    QTableView* view = panel_->tableView();
    view->setModel(model_);
    QHeaderView* header = view->horizontalHeader();

    connect(panel_, &TablePanel::saveRequested, this, &TablePanelView::saveTable);
    connect(panel_, &TablePanel::modifyTableRequested, this, &TablePanelView::modifyTable);
    // A saved table deleted from the Tables panel becomes an unsaved table.
    connect(&store_, &TableStore::changed, this, [this] {
        if (!current_id_.isEmpty() && !store_.table(current_id_)) current_id_.clear();
        updateSummary();
    });
    connect(panel_, &TablePanel::addColumnRequested, this,
            [this] { addColumn(table_ ? static_cast<int>(table_->columns.size()) : 0); });
    connect(panel_, &TablePanel::refreshRequested, this, &TablePanelView::evaluate);
    connect(panel_, &TablePanel::exportRequested, this, &TablePanelView::exportCsv);
    connect(header, &QHeaderView::sectionClicked, this, &TablePanelView::showHeaderMenu);
    connect(header, &QHeaderView::sectionMoved, this, [this](int, int old_visual, int new_visual) {
        // Deferred: the header is still inside its move when this fires.
        QTimer::singleShot(0, this, [this, old_visual, new_visual] { moveColumn(old_visual, new_visual); });
    });
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TablePanelView::selectRows);
    connect(view, &QTableView::doubleClicked, this, [this](const QModelIndex& index) { isolateRow(index.row()); });
}

// ---------------------------------------------------------------------------
// Table and columns

void TablePanelView::newTable(const QString& folder_id) {
    TableDefinition definition;
    definition.columns = defaultColumns();
    if (!editTable(definition)) return;
    table_ = definition;
    current_id_.clear();
    pending_folder_ = folder_id;
    model_->setRows({});
    applyColumns();
    // Created from the Tables panel: stored at once, so it appears in the tree.
    saveTable();
    panel_->setTableActionsEnabled(true);
    evaluate();
}

void TablePanelView::saveTable() {
    if (!table_) return;
    TableDocument document;
    document.id = current_id_;
    document.definition = *table_;
    if (current_id_.isEmpty()) {
        current_id_ = store_.addTable(document, pending_folder_);
    } else {
        store_.updateTable(document);
    }
    updateSummary();
    session_.setStatusMessage("Table", QString("Table “%1” saved").arg(table_->name));
}

void TablePanelView::openTable(const QString& table_id) {
    const auto document = store_.table(table_id);
    if (!document) return;
    table_ = document->definition;
    current_id_ = table_id;
    pending_folder_ = document->folder_id;
    model_->setRows({});
    applyColumns();
    panel_->setTableActionsEnabled(true);
    evaluate();
}

void TablePanelView::modifyTable() {
    if (!table_) return;
    TableDefinition definition = *table_;
    if (!editTable(definition)) return;
    table_ = definition;
    evaluate();
}

bool TablePanelView::editTable(TableDefinition& definition) {
    TableCreationDialog dialog(definition, bridge_, [this] { return ifcPath(activeSessionModelId()); },
                               panel_);
    connect(&dialog, &TableCreationDialog::isolateRequested, this, [this](const std::vector<int>& express_ids) {
        isolateExpressIds(activeSessionModelId(), express_ids);
    });
    if (dialog.exec() != QDialog::Accepted) return false;
    definition = dialog.definition();
    return true;
}

void TablePanelView::addColumn(int position) {
    if (!table_) return;
    ColumnDialog dialog(TableColumn{}, catalog_, true, panel_);
    if (dialog.exec() != QDialog::Accepted) return;
    position = std::clamp(position, 0, static_cast<int>(table_->columns.size()));
    table_->columns.insert(table_->columns.begin() + position, dialog.column());
    applyColumns();
}

void TablePanelView::modifyColumn(int index) {
    if (!table_ || index < 0 || index >= static_cast<int>(table_->columns.size())) return;
    ColumnDialog dialog(table_->columns[static_cast<size_t>(index)], catalog_, false, panel_);
    if (dialog.exec() != QDialog::Accepted) return;
    table_->columns[static_cast<size_t>(index)] = dialog.column();
    applyColumns();
}

void TablePanelView::removeColumn(int index) {
    if (!table_ || index < 0 || index >= static_cast<int>(table_->columns.size())) return;
    table_->columns.erase(table_->columns.begin() + index);
    applyColumns();
}

void TablePanelView::showHeaderMenu(int section) {
    if (!table_) return;
    QMenu menu(panel_);
    menu.addAction("Modify Column…", this, [this, section] { modifyColumn(section); });
    menu.addAction("Remove Column", this, [this, section] { removeColumn(section); });
    menu.addAction("Add Column to the Right…", this, [this, section] { addColumn(section + 1); });
    menu.exec(QCursor::pos());
}

void TablePanelView::moveColumn(int from, int to) {
    if (!table_ || from == to) return;
    const int count = static_cast<int>(table_->columns.size());
    if (from < 0 || from >= count || to < 0 || to >= count) return;
    // Undo the header's own move (visual order = column order again), then
    // move the column in the table itself.
    {
        QHeaderView* header = panel_->tableView()->horizontalHeader();
        const QSignalBlocker blocker(header);
        header->moveSection(to, from);
    }
    const TableColumn column = table_->columns[static_cast<size_t>(from)];
    table_->columns.erase(table_->columns.begin() + from);
    table_->columns.insert(table_->columns.begin() + to, column);
    applyColumns();
}

void TablePanelView::applyColumns() {
    model_->setColumns(table_ ? table_->columns : std::vector<TableColumn>{});
    updateSummary();
}

// ---------------------------------------------------------------------------
// Evaluation on all the loaded models

void TablePanelView::evaluate() {
    if (!table_) return;
    ++generation_;
    sources_.clear();
    for (std::uint32_t session_model_id : session_.sessionModelIds()) {
        const QString path = ifcPath(session_model_id);
        if (!path.isEmpty() && QFileInfo::exists(path)) sources_.push_back({session_model_id, path});
    }
    source_index_ = 0;
    found_.clear();
    errors_.clear();
    evaluating_ = true;
    updateSummary();
    evaluateNext();
}

void TablePanelView::evaluateNext() {
    if (source_index_ >= sources_.size()) {
        finishEvaluation();
        return;
    }
    const ModelSource source = sources_[source_index_];
    const int generation = generation_;
    QPointer<TablePanelView> self(this);
    bridge_.preview(source.ifc_path, table_->selection,
                    [self, generation, source](bool ok, std::vector<int> express_ids, const QString& error) {
                        if (self == nullptr || generation != self->generation_) return;
                        if (ok) {
                            const auto ids = self->objectIds(source.session_model_id, express_ids);
                            self->found_.insert(self->found_.end(), ids.begin(), ids.end());
                        } else {
                            self->errors_.append(QString("%1: %2").arg(QFileInfo(source.ifc_path).fileName(), error));
                        }
                        ++self->source_index_;
                        self->evaluateNext();
                    });
}

void TablePanelView::finishEvaluation() {
    evaluating_ = false;
    model_->setRows(found_);
    updateSummary();
    if (!errors_.isEmpty()) {
        services::popup::information(panel_, "The table could not be evaluated on every model:\n"
                                                 + errors_.join('\n'));
    }
}

void TablePanelView::updateSummary() {
    if (!table_) {
        panel_->setSummary("No table loaded. Create or run one in the Tables panel.");
        return;
    }
    QString text = QString("<b>%1</b>").arg(table_->name.toHtmlEscaped());
    if (current_id_.isEmpty()) text += " <i>(not saved)</i>";
    if (evaluating_) {
        text += QString(" — evaluating %1 model(s)…").arg(sources_.size());
    } else {
        text += QString(" — %1 element(s) in %2 model(s)").arg(model_->rowCount()).arg(sources_.size());
    }
    panel_->setSummary(text);
    panel_->tableView()->setToolTip(table_->description);
}

// ---------------------------------------------------------------------------
// Interactions

void TablePanelView::selectRows() {
    std::set<int> rows;
    for (const QModelIndex& index : panel_->tableView()->selectionModel()->selectedIndexes()) {
        rows.insert(index.row());
    }
    std::vector<std::uint32_t> ids;
    ids.reserve(rows.size());
    for (int row : rows) ids.push_back(model_->objectId(row));
    selection_.setSelection(ids, services::SelectionSource::Table);
}

void TablePanelView::isolateRow(int row) {
    if (row < 0 || row >= model_->rowCount()) return;
    actions_.run({model_->objectId(row)}, services::ActionPipeline::Action::Isolate);
}

void TablePanelView::isolateExpressIds(std::uint32_t session_model_id, const std::vector<int>& express_ids) {
    actions_.run(objectIds(session_model_id, express_ids), services::ActionPipeline::Action::Isolate);
}

void TablePanelView::exportCsv() {
    if (!table_) return;
    QString file_name = table_->name;
    file_name.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    const QString path = QFileDialog::getSaveFileName(panel_, "Export the Table", file_name + ".csv",
                                                      "CSV files (*.csv)");
    if (path.isEmpty()) return;

    // Follow the system locale, so the file opens as columns in a
    // spreadsheet: "," decimals go with ";" separators.
    const QChar decimal_point = QLocale().decimalPoint().at(0);
    const QChar separator = decimal_point == ',' ? ';' : ',';
    QStringList lines;
    QStringList header;
    for (int column = 0; column < model_->columnCount(); ++column) {
        header.append(csvField(model_->headerData(column, Qt::Horizontal), separator, decimal_point));
    }
    lines.append(header.join(separator));
    for (int row = 0; row < model_->rowCount(); ++row) {
        QStringList fields;
        for (int column = 0; column < model_->columnCount(); ++column) {
            fields.append(csvField(model_->value(row, column), separator, decimal_point));
        }
        lines.append(fields.join(separator));
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        services::popup::information(panel_, QString("Cannot write %1: %2").arg(path, file.errorString()));
        return;
    }
    file.write("\xEF\xBB\xBF"); // UTF-8 BOM, for spreadsheet applications
    file.write((lines.join("\r\n") + "\r\n").toUtf8());
    if (!file.commit()) {
        services::popup::information(panel_, QString("Cannot write %1: %2").arg(path, file.errorString()));
    }
}

// ---------------------------------------------------------------------------
// Models and ids

std::vector<std::uint32_t> TablePanelView::objectIds(std::uint32_t session_model_id,
                                                     const std::vector<int>& express_ids) const {
    QHash<int, std::uint32_t> object_id_by_ifc_id;
    for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
        object_id_by_ifc_id.insert(info.ifc_id, info.object_id);
    }
    std::vector<std::uint32_t> object_ids;
    object_ids.reserve(express_ids.size());
    for (int id : express_ids) {
        const auto found = object_id_by_ifc_id.constFind(id);
        if (found != object_id_by_ifc_id.cend()) object_ids.push_back(found.value());
    }
    return object_ids;
}

std::uint32_t TablePanelView::activeSessionModelId() const {
    const QString model_id = session_.activeModelId();
    return model_id.isEmpty() ? 0 : session_.sessionModelIdForModelId(model_id);
}

QString TablePanelView::ifcPath(std::uint32_t session_model_id) const {
    auto* loader = session_.loader();
    return (loader != nullptr && session_model_id != 0) ? loader->filePath(session_model_id) : QString();
}

} // namespace bonsaiviewer::modules::table
