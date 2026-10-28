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
#include "TableHeader.h"
#include "TableModel.h"
#include "TableProxy.h"
#include "TableStore.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../services/ActionPipeline.h"
#include "../../services/AppearanceService.h"
#include "../../services/EntitySetStore.h"
#include "../../services/PopupService.h"
#include "../../services/PropertyCatalog.h"
#include "../../services/SelectionService.h"
#include "../../../ifcviewer/SceneLoader.h"

#include <QCursor>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QPointer>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QTableView>
#include <QTimer>

#include <algorithm>
#include <set>
#include <unordered_set>

namespace bonsaiviewer::modules::table {

namespace {

std::vector<TableColumn> defaultColumns() {
    TableColumn name;
    name.kind = TableColumn::Kind::Name;
    TableColumn ifc_class;
    ifc_class.kind = TableColumn::Kind::Class;
    return {name, ifc_class};
}

// A pleasant, not too dark random colour for a Color column row.
QColor randomColour() {
    return QColor::fromHsv(QRandomGenerator::global()->bounded(360), 170, 235);
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
                               services::AppearanceService& appearance,
                               services::EntitySetStore& box,
                               QObject* parent)
    : QObject(parent),
      panel_(panel),
      session_(session),
      bridge_(bridge),
      data_access_(data_access),
      catalog_(catalog),
      selection_(selection),
      actions_(actions),
      store_(store),
      appearance_(appearance),
      box_(box)
{
    model_ = new TableModel(
        data_access,
        [this](std::uint32_t session_model_id) { return QFileInfo(ifcPath(session_model_id)).fileName(); },
        this);
    proxy_ = new TableProxy(model_, this);
    QTableView* view = panel_->tableView();
    view->setModel(proxy_);
    QHeaderView* header = view->horizontalHeader();
    connect(panel_->tableHeader(), &TableHeader::sortRequested, this, &TablePanelView::toggleSort);
    connect(panel_->tableHeader(), &TableHeader::filterRequested, this, &TablePanelView::showFilterMenu);
    // Rows shown or hidden by a filter change the row count.
    connect(proxy_, &QAbstractItemModel::rowsInserted, this, &TablePanelView::updateSummary);
    connect(proxy_, &QAbstractItemModel::rowsRemoved, this, &TablePanelView::updateSummary);
    connect(proxy_, &QAbstractItemModel::modelReset, this, &TablePanelView::updateSummary);

    connect(panel_, &TablePanel::saveRequested, this, &TablePanelView::saveTable);
    connect(panel_, &TablePanel::modifyTableRequested, this, &TablePanelView::modifyTable);
    // A saved table deleted from the Tables panel becomes an unsaved table.
    connect(&store_, &TableStore::changed, this, [this] {
        if (!current_id_.isEmpty() && !store_.table(current_id_)) current_id_.clear();
        updateSummary();
    });
    // Run on Box: the run follows the box while it is set.
    connect(&box_, &services::EntitySetStore::currentChanged, this, [this] {
        if (table_ && apply_to_box_) evaluate();
    });
    connect(panel_, &TablePanel::addColumnRequested, this,
            [this] { addColumn(table_ ? static_cast<int>(table_->columns.size()) : 0); });
    connect(panel_, &TablePanel::refreshRequested, this, &TablePanelView::evaluate);
    connect(panel_, &TablePanel::exportRequested, this, &TablePanelView::exportCsv);
    // Right click on a column header = column menu; a left drag still moves the column.
    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QWidget::customContextMenuRequested, this, [this, header](const QPoint& pos) {
        showHeaderMenu(header->logicalIndexAt(pos));
    });
    connect(header, &QHeaderView::sectionMoved, this, [this](int, int old_visual, int new_visual) {
        // Deferred: the header is still inside its move when this fires.
        QTimer::singleShot(0, this, [this, old_visual, new_visual] { moveColumn(old_visual, new_visual); });
    });
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TablePanelView::selectRows);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QTableView::doubleClicked, this,
            [this](const QModelIndex& index) { isolateRows(rowsForAction(sourceRow(index.row()))); });
    connect(view, &QWidget::customContextMenuRequested, this, [this, view](const QPoint& pos) {
        const int row = sourceRow(view->indexAt(pos).row());
        QMenu menu(panel_);
        menu.addAction("Isolate", this, [this, row] { isolateRows(rowsForAction(row)); });
        menu.exec(view->viewport()->mapToGlobal(pos));
    });
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
    apply_to_box_ = false;
    temporary_ = false;
    model_->setRows({});
    applyColumns();
    // Created from the Tables panel: stored at once, so it appears in the tree.
    saveTable();
    panel_->setTableActionsEnabled(true);
    evaluate();
}

void TablePanelView::saveTable() {
    if (temporary_) {
        services::popup::information(panel_, "A temporary table is not saved.");
        return;
    }
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

void TablePanelView::openTable(const QString& table_id, bool on_box) {
    const auto document = store_.table(table_id);
    if (!document) return;
    table_ = document->definition;
    current_id_ = table_id;
    pending_folder_ = document->folder_id;
    apply_to_box_ = on_box;
    temporary_ = false;
    model_->setRows({});
    applyColumns();
    panel_->setTableActionsEnabled(true);
    evaluate();
}

void TablePanelView::showTemporaryTable(const QString& name, const std::vector<TableColumn>& columns,
                                        const std::vector<std::uint32_t>& ids,
                                        const std::unordered_map<std::uint32_t, QColor>& object_colours) {
    // A pending evaluation must not overwrite the temporary rows.
    ++generation_;
    evaluating_ = false;
    TableDefinition definition;
    definition.name = name;
    definition.description = "Temporary table: not saved, not run on the IDS selection.";
    definition.columns = columns;
    table_ = definition;
    current_id_.clear();
    pending_folder_.clear();
    apply_to_box_ = false;
    temporary_ = true;
    temporary_colours_ = object_colours;
    panel_->setTableActionsEnabled(true);
    applyColumns();
    model_->setRows(ids);
    applyRowColours();
    updateSummary();
}

void TablePanelView::modifyTable() {
    if (!table_) return;
    TableDefinition definition = *table_;
    if (!editTable(definition)) return;
    table_ = definition;
    // The model regroups its rows from its columns: they must be current.
    applyColumns();
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

// Browse… of the column's property: "Only the elements of the table".
void TablePanelView::offerTableScope(ColumnDialog& dialog) {
    const std::vector<std::uint32_t> elements = model_->elements();
    if (elements.empty()) return;
    dialog.setScopedSource(
        QString("the %1 element(s) of the table").arg(static_cast<qulonglong>(elements.size())),
        [this, elements](std::function<void(const property_selection::PropertyPicker::PropertyMap&,
                                            const property_selection::PropertyPicker::SampleMap&)> done) {
            property_selection::PropertyPicker::PropertyMap psets;
            property_selection::PropertyPicker::SampleMap samples;
            services::PropertyCatalog::collect(data_access_, elements, psets, samples);
            done(psets, samples);
        });
}

void TablePanelView::addColumn(int position) {
    if (!table_) return;
    ColumnDialog dialog(TableColumn{}, catalog_, true, panel_);
    offerTableScope(dialog);
    if (dialog.exec() != QDialog::Accepted) return;
    position = std::clamp(position, 0, static_cast<int>(table_->columns.size()));
    table_->columns.insert(table_->columns.begin() + position, dialog.column());
    applyColumns();
}

void TablePanelView::modifyColumn(int index) {
    if (!table_ || index < 0 || index >= static_cast<int>(table_->columns.size())) return;
    ColumnDialog dialog(table_->columns[static_cast<size_t>(index)], catalog_, false, panel_);
    offerTableScope(dialog);
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
    if (!table_ || section < 0) return;
    QMenu menu(panel_);
    menu.addAction("Modify Column…", this, [this, section] { modifyColumn(section); });
    menu.addAction("Remove Column", this, [this, section] { removeColumn(section); });
    menu.addAction("Add Column to the Right…", this, [this, section] { addColumn(section + 1); });
    menu.exec(QCursor::pos());
}

void TablePanelView::toggleSort(int section) {
    if (!table_ || section < 0) return;
    if (section != sort_column_) {
        sort_column_ = section;
        sort_order_ = Qt::AscendingOrder;
    } else if (sort_order_ == Qt::AscendingOrder) {
        sort_order_ = Qt::DescendingOrder;
    } else {
        sort_column_ = -1;
    }
    proxy_->sort(sort_column_, sort_order_);
    panel_->tableHeader()->setSortState(sort_column_, sort_order_);
}

void TablePanelView::showFilterMenu(int section, const QPoint& global_pos) {
    if (!table_ || section < 0) return;
    const QString empty_label = "(empty)";
    const QSet<QString> kept = proxy_->columnFilter(section);
    const bool filtered = proxy_->isFiltered(section);

    QMenu menu(panel_);
    auto* body = new QWidget(&menu);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(6, 6, 6, 6);
    auto* search = new QLineEdit(body);
    search->setPlaceholderText("Search…");
    search->setClearButtonEnabled(true);
    layout->addWidget(search);
    auto* list = new QListWidget(body);
    list->setMinimumSize(220, 220);
    for (const QString& value : proxy_->columnValues(section)) {
        auto* item = new QListWidgetItem(value.isEmpty() ? empty_label : value, list);
        item->setData(Qt::UserRole, value);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(!filtered || kept.contains(value) ? Qt::Checked : Qt::Unchecked);
    }
    layout->addWidget(list);
    auto* choice_row = new QHBoxLayout();
    auto* all_button = new QPushButton("All", body);
    auto* none_button = new QPushButton("None", body);
    choice_row->addWidget(all_button);
    choice_row->addWidget(none_button);
    choice_row->addStretch(1);
    layout->addLayout(choice_row);
    auto* action_row = new QHBoxLayout();
    auto* clear_button = new QPushButton("Clear Filter", body);
    auto* apply_button = new QPushButton("Apply", body);
    apply_button->setDefault(true);
    action_row->addWidget(clear_button);
    action_row->addStretch(1);
    action_row->addWidget(apply_button);
    layout->addLayout(action_row);
    auto* action = new QWidgetAction(&menu);
    action->setDefaultWidget(body);
    menu.addAction(action);

    // The search narrows the list; All / None apply to the listed values.
    connect(search, &QLineEdit::textChanged, list, [list](const QString& text) {
        for (int i = 0; i < list->count(); ++i) {
            list->item(i)->setHidden(!list->item(i)->text().contains(text, Qt::CaseInsensitive));
        }
    });
    const auto check_listed = [list](Qt::CheckState state) {
        for (int i = 0; i < list->count(); ++i) {
            if (!list->item(i)->isHidden()) list->item(i)->setCheckState(state);
        }
    };
    connect(all_button, &QPushButton::clicked, list, [check_listed] { check_listed(Qt::Checked); });
    connect(none_button, &QPushButton::clicked, list, [check_listed] { check_listed(Qt::Unchecked); });
    connect(clear_button, &QPushButton::clicked, &menu, [this, section, &menu] {
        proxy_->clearColumnFilter(section);
        panel_->tableHeader()->setFiltered(section, false);
        menu.close();
    });
    connect(apply_button, &QPushButton::clicked, &menu, [this, section, list, &menu] {
        QSet<QString> values;
        bool all = true;
        for (int i = 0; i < list->count(); ++i) {
            if (list->item(i)->checkState() == Qt::Checked) values.insert(list->item(i)->data(Qt::UserRole).toString());
            else all = false;
        }
        // Every value checked = no filter.
        if (all) proxy_->clearColumnFilter(section);
        else proxy_->setColumnFilter(section, values);
        panel_->tableHeader()->setFiltered(section, !all);
        menu.close();
    });
    search->setFocus();
    menu.exec(global_pos);
}

void TablePanelView::clearSortAndFilters() {
    sort_column_ = -1;
    proxy_->sort(-1);
    proxy_->clearFilters();
    panel_->tableHeader()->clearStates();
}

int TablePanelView::sourceRow(int displayed_row) const {
    return proxy_->sourceRow(displayed_row);
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
    clearSortAndFilters();
    // Regroups the rows when a group-key column changes.
    model_->setColumns(table_ ? table_->columns : std::vector<TableColumn>{});
    applyRowColours();
    updateSummary();
}

// One colour per row; a row's colour goes to all the elements of the row.
void TablePanelView::applyRowColours() {
    if (temporary_) {
        // The viewer already shows these colours (set by the caller): the
        // cells only display them, the viewer is not painted again.
        std::vector<QColor> colours;
        colours.reserve(static_cast<size_t>(model_->rowCount()));
        for (int row = 0; row < model_->rowCount(); ++row) {
            const auto& objects = model_->objects(row);
            const auto found = objects.empty() ? temporary_colours_.end()
                                               : temporary_colours_.find(objects.front());
            colours.push_back(found != temporary_colours_.end() ? found->second : QColor(150, 150, 150));
        }
        model_->setRowColours(colours);
        return;
    }
    appearance_.clearLayer(services::AppearanceService::Layer::TableColour);
    const bool has_colour = table_ && std::any_of(table_->columns.begin(), table_->columns.end(),
                                                  [](const TableColumn& column) {
                                                      return column.kind == TableColumn::Kind::Color;
                                                  });
    std::vector<QColor> colours;
    if (has_colour) {
        colours.reserve(static_cast<size_t>(model_->rowCount()));
        for (int row = 0; row < model_->rowCount(); ++row) {
            const QColor colour = randomColour();
            colours.push_back(colour);
            appearance_.setLayerColour(model_->objects(row), colour,
                                       services::AppearanceService::Layer::TableColour);
        }
    }
    model_->setRowColours(colours);
}

// ---------------------------------------------------------------------------
// Evaluation on all the loaded models

void TablePanelView::evaluate() {
    if (!table_ || temporary_) return;
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
    if (apply_to_box_) {
        const auto& box = box_.current();
        const std::unordered_set<std::uint32_t> in_box(box.begin(), box.end());
        found_.erase(std::remove_if(found_.begin(), found_.end(),
                                    [&in_box](std::uint32_t id) { return in_box.count(id) == 0; }),
                     found_.end());
    }
    model_->setRows(found_);
    applyRowColours();
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
    if (temporary_) {
        text += " <i>(temporary)</i>";
    } else if (current_id_.isEmpty()) {
        text += " <i>(not saved)</i>";
    }
    if (apply_to_box_) text += " <i>(on the box)</i>";
    // "12 of 40 row(s)" while a filter hides rows.
    const QString rows = proxy_->rowCount() == model_->rowCount()
                             ? QString::number(model_->rowCount())
                             : QString("%1 of %2").arg(proxy_->rowCount()).arg(model_->rowCount());
    if (temporary_) {
        text += QString(" — %1 row(s)").arg(rows);
    } else if (evaluating_) {
        text += QString(" — evaluating %1 model(s)…").arg(sources_.size());
    } else {
        text += QString(" — %1 row(s) from %2 model(s)").arg(rows).arg(sources_.size());
    }
    panel_->setSummary(text);
    panel_->tableView()->setToolTip(table_->description);
}

// ---------------------------------------------------------------------------
// Interactions

void TablePanelView::selectRows() {
    std::set<int> rows;
    for (const QModelIndex& index : panel_->tableView()->selectionModel()->selectedIndexes()) {
        rows.insert(sourceRow(index.row()));
    }
    std::vector<std::uint32_t> ids;
    for (int row : rows) {
        const auto& objects = model_->objects(row);
        ids.insert(ids.end(), objects.begin(), objects.end());
    }
    selection_.setSelection(ids, services::SelectionSource::Table);
}

std::vector<int> TablePanelView::rowsForAction(int clicked_row) const {
    std::set<int> rows;
    for (const QModelIndex& index : panel_->tableView()->selectionModel()->selectedIndexes()) {
        rows.insert(sourceRow(index.row()));
    }
    if (clicked_row >= 0 && rows.count(clicked_row) == 0) return {clicked_row};
    return {rows.begin(), rows.end()};
}

void TablePanelView::isolateRows(const std::vector<int>& rows) {
    std::vector<std::uint32_t> ids;
    for (int row : rows) {
        if (row < 0 || row >= model_->rowCount()) continue;
        const auto& objects = model_->objects(row);
        ids.insert(ids.end(), objects.begin(), objects.end());
    }
    if (!ids.empty()) actions_.run(ids, services::ActionPipeline::Action::Isolate);
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
    // The displayed rows, in the displayed order (sort and filters).
    for (int displayed = 0; displayed < proxy_->rowCount(); ++displayed) {
        const int row = sourceRow(displayed);
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
