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

#include "View.h"

#include "Dialogs.h"
#include "Panel.h"
#include "GridPanel.h"
#include "PlanOverlay.h"

#include "../../SessionState.h"
#include "../../ElementRegistry.h"
#include "../../services/ActiveView.h"
#include "../../services/IfcDataAccess.h"
#include "../../services/PropertyCatalog.h"
#include "../../services/SelectionService.h"
#include "../../services/UndoStack.h"
#include "../../../ifcviewer/Federation.h"
#include "../../../ifcviewer/ViewportWindow.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <cmath>
#include <functional>

namespace bonsaiviewer::modules::external_documents {

namespace {

// The .ifcfed extension key of the External Documents section.
constexpr const char* kExtensionKey = "bonsaiviewer_external_documents";

constexpr int kRoleId = Qt::UserRole;
constexpr int kRoleKind = Qt::UserRole + 1; // "folder" | "table" | "plan"

QString itemKind(QTreeWidgetItem* item) {
    return item != nullptr ? item->data(0, kRoleKind).toString() : QString();
}

QString documentFilter() {
    return QStringLiteral("External documents (*.csv *.xlsx *.pdf *.png *.jpg *.jpeg *.svg *.dxf *.dwg);;"
                          "Tables (*.csv *.xlsx);;2D plans (*.pdf *.png *.jpg *.jpeg *.svg *.dxf *.dwg);;"
                          "All Files (*)");
}

} // namespace

ExternalDocumentsView::ExternalDocumentsView(SessionState& session, ViewportWindow& viewport,
                                             services::IfcDataAccess& data_access,
                                             services::SelectionService& selection,
                                             services::ActiveView& active_view,
                                             services::UndoStack& undo_stack,
                                             services::PropertyCatalog& catalog,
                                             ExternalDocumentsPanel* panel,
                                             ExternalGridPanel* grid,
                                             QObject* parent)
    : QObject(parent), session_(session), viewport_(viewport), data_access_(data_access),
      selection_(selection), active_view_(active_view), undo_stack_(undo_stack),
      catalog_(catalog), panel_(panel), grid_(grid)
{
    auto* tree = panel_->treeWidget();
    tree->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(panel_, &ExternalDocumentsPanel::addFolderRequested, this, &ExternalDocumentsView::addFolder);
    connect(panel_, &ExternalDocumentsPanel::importRequested, this, &ExternalDocumentsView::importDocuments);
    connect(tree, &QTreeWidget::customContextMenuRequested, this, [this, tree](const QPoint& pos) {
        showItemContextMenu(tree->itemAt(pos), tree->viewport()->mapToGlobal(pos));
    });
    connect(tree, &QTreeWidget::itemClicked, this, &ExternalDocumentsView::onItemClicked);
    // Move by drag and drop: once the tree moved the items, sync the store
    // (deferred: rebuilding the tree inside rowsMoved would be re-entrant).
    connect(tree->model(), &QAbstractItemModel::rowsMoved, this,
            [this](const QModelIndex& source_parent, int start, int end, const QModelIndex& destination, int dest_row) {
        Q_UNUSED(source_parent);
        auto* tree = panel_->treeWidget();
        QTreeWidgetItem* dest_parent = destination.isValid() ? tree->itemFromIndex(destination)
                                                             : tree->invisibleRootItem();
        QString folder_id;
        if (dest_parent != nullptr && dest_parent != tree->invisibleRootItem() && itemKind(dest_parent) == QLatin1String("folder")) {
            folder_id = dest_parent->data(0, kRoleId).toString();
        }
        QStringList folder_ids, document_ids;
        for (int r = start; r <= end; ++r) {
            const QModelIndex index = tree->model()->index(dest_row + (r - start), 0, destination);
            QTreeWidgetItem* item = tree->itemFromIndex(index);
            if (item == nullptr) continue;
            const QString kind = itemKind(item);
            if (kind == QLatin1String("folder")) folder_ids.append(item->data(0, kRoleId).toString());
            else if (kind == QLatin1String("table") || kind == QLatin1String("plan"))
                document_ids.append(item->data(0, kRoleId).toString());
        }
        QTimer::singleShot(0, this, [this, folder_id, folder_ids, document_ids]() {
            for (const QString& id : folder_ids) store_.moveFolder(id, folder_id);
            for (const QString& id : document_ids) store_.moveDocument(id, folder_id);
        });
    });
    connect(&store_, &DocumentStore::structureChanged, this, [this]() {
        rebuildTree();
        if (auto* f = session_.federation()) f->markDirty();
    });
    connect(&store_, &DocumentStore::documentChanged, this, [this](const QString& id) {
        rebuildTree();
        if (auto* document = store_.document(id)) {
            if (document->kind == DocumentKind::Plan) updatePlanOverlays();
        }
        if (auto* f = session_.federation()) f->markDirty();
    });

    // Project persistence.
    connect(&session_, &SessionState::projectAboutToSave, this, &ExternalDocumentsView::saveToFederation);
    connect(&session_, &SessionState::projectOpened, this, [this](const QString&) { restoreFromFederation(); });
    connect(&session_, &SessionState::projectReset, this, [this]() {
        store_.clear();
        cache_.clear();
        if (grid_ != nullptr) grid_->clearTable();
        updatePlanOverlays();
    });

    // Grid interactions.
    connect(grid_, &ExternalGridPanel::rowsSelected, this, [this](const QList<std::uint32_t>& ids, bool add) {
        std::vector<std::uint32_t> vec(ids.begin(), ids.end());
        selection_.setSelection(vec, services::SelectionSource::Table,
                                add ? services::SelectionMode::Add : services::SelectionMode::Replace);
    });
    connect(grid_, &ExternalGridPanel::rowsIsolateRequested, this, [this](const QList<std::uint32_t>& ids) {
        std::vector<std::uint32_t> vec(ids.begin(), ids.end());
        active_view_.isolate(vec);
    });
    connect(grid_, &ExternalGridPanel::refreshRequested, this, [this]() {
        if (ExternalDocument* document = store_.document(grid_->documentId())) refresh(*document);
    });
    connect(grid_, &ExternalGridPanel::editJoinRequested, this, [this]() {
        if (ExternalDocument* document = store_.document(grid_->documentId())) editJoin(*document);
    });
    connect(grid_, &ExternalGridPanel::exportCsvRequested, this, [this]() {
        if (ExternalDocument* document = store_.document(grid_->documentId())) exportCsv(*document);
    });
    connect(grid_, &ExternalGridPanel::showReportRequested, this, [this]() {
        if (ExternalDocument* document = store_.document(grid_->documentId())) showReport(*document);
    });

    rebuildTree();
}

// --- tree ---------------------------------------------------------------------

void ExternalDocumentsView::rebuildTree() {
    auto* tree = panel_->treeWidget();
    tree->clear();
    // Folders, parents first; then the documents of each folder.
    std::function<void(QTreeWidgetItem*, const QString&)> addFolders =
        [&](QTreeWidgetItem* parent_item, const QString& parent_id) {
            for (const FolderNode& folder : store_.folders()) {
                if (folder.parent_id != parent_id) continue;
                QTreeWidgetItem* item = addFolderItem(parent_item, folder);
                addFolders(item, folder.id);
            }
        };
    addFolders(tree->invisibleRootItem(), QString());
    for (const ExternalDocument& document : store_.documents()) {
        QTreeWidgetItem* parent_item = tree->invisibleRootItem();
        if (!document.folder_id.isEmpty()) {
            // Find the folder item by id.
            std::function<QTreeWidgetItem*(QTreeWidgetItem*)> find =
                [&](QTreeWidgetItem* item) -> QTreeWidgetItem* {
                    for (int i = 0; i < item->childCount(); ++i) {
                        QTreeWidgetItem* child = item->child(i);
                        if (child->data(0, kRoleKind) == QLatin1String("folder") &&
                            child->data(0, kRoleId).toString() == document.folder_id) return child;
                        if (QTreeWidgetItem* hit = find(child)) return hit;
                    }
                    return nullptr;
                };
            if (QTreeWidgetItem* hit = find(tree->invisibleRootItem())) parent_item = hit;
        }
        addDocumentItem(parent_item, document);
    }
    tree->expandAll();
}

QTreeWidgetItem* ExternalDocumentsView::addFolderItem(QTreeWidgetItem* parent, const FolderNode& folder) {
    auto* item = new QTreeWidgetItem(parent, {folder.name});
    item->setIcon(0, QIcon(QStringLiteral(":/icons/folder.svg")));
    item->setData(0, kRoleId, folder.id);
    item->setData(0, kRoleKind, QStringLiteral("folder"));
    return item;
}

void ExternalDocumentsView::addDocumentItem(QTreeWidgetItem* parent, const ExternalDocument& document) {
    QString label = document.name;
    if (document.state == DocumentState::FileNotFound) label += QStringLiteral(" — file not found");
    else if (document.state == DocumentState::KeyColumnMissing) label += QStringLiteral(" — key column missing");
    else if (document.kind == DocumentKind::Plan && !document.plan.visible) label += QStringLiteral(" — hidden");
    auto* item = new QTreeWidgetItem(parent, {label});
    item->setIcon(0, QIcon(document.kind == DocumentKind::Table ? QStringLiteral(":/icons/database.svg")
                                                                : QStringLiteral(":/icons/planimetry.svg")));
    item->setData(0, kRoleId, document.id);
    item->setData(0, kRoleKind, document.kind == DocumentKind::Table ? QStringLiteral("table") : QStringLiteral("plan"));
}

QString ExternalDocumentsView::selectedFolderId() {
    const QTreeWidgetItem* item = panel_->treeWidget()->currentItem();
    if (item == nullptr) return {};
    if (itemKind(const_cast<QTreeWidgetItem*>(item)) == QLatin1String("folder"))
        return item->data(0, kRoleId).toString();
    // A document: its parent folder.
    const QString doc_id = item->data(0, kRoleId).toString();
    if (const ExternalDocument* document = store_.document(doc_id)) return document->folder_id;
    return {};
}

ExternalDocument* ExternalDocumentsView::selectedDocument() {
    QTreeWidgetItem* item = panel_->treeWidget()->currentItem();
    if (item == nullptr) return nullptr;
    const QString kind = itemKind(item);
    if (kind != QLatin1String("table") && kind != QLatin1String("plan")) return nullptr;
    return store_.document(item->data(0, kRoleId).toString());
}

void ExternalDocumentsView::onItemClicked(QTreeWidgetItem* item) {
    // Left-click on a plan opens its menu: Modify Plan, Show / Hide, Delete.
    if (item == nullptr || itemKind(item) != QLatin1String("plan")) return;
    if (ExternalDocument* document = store_.document(item->data(0, kRoleId).toString())) {
        showPlanClickMenu(*document, QCursor::pos());
    }
}

// --- context menus --------------------------------------------------------------

void ExternalDocumentsView::showItemContextMenu(QTreeWidgetItem* item, const QPoint& global_pos) {
    const QString kind = itemKind(item);
    if (kind == QLatin1String("table") || kind == QLatin1String("plan")) {
        ExternalDocument* document = store_.document(item->data(0, kRoleId).toString());
        if (document == nullptr) return;
        QMenu menu(panel_);
        if (document->kind == DocumentKind::Table) {
            menu.addAction(QStringLiteral("Visualize"), this, [this, document]() { visualize(*document); });
            menu.addAction(QStringLiteral("Refresh"), this, [this, document]() { refresh(*document); });
            menu.addAction(QStringLiteral("Edit Join"), this, [this, document]() { editJoin(*document); });
            menu.addSeparator();
            menu.addAction(QStringLiteral("Locate File…"), this, [this, document]() { locateFile(*document); });
            menu.addAction(QStringLiteral("Delete"), this, [this, document]() { removeDocument(*document); });
        } else {
            menu.addAction(document->plan.visible ? QStringLiteral("Hide") : QStringLiteral("Show"),
                           this, [this, document]() { togglePlanVisibility(*document); });
            menu.addAction(QStringLiteral("Edit Plan"), this, [this, document]() { editPlan(*document); });
            menu.addAction(QStringLiteral("Refresh"), this, [this, document]() {
                loadDocument(*document);
                updatePlanOverlays();
            });
            menu.addSeparator();
            menu.addAction(QStringLiteral("Locate File…"), this, [this, document]() { locateFile(*document); });
            menu.addAction(QStringLiteral("Export Config (JSON)"), this, [this, document]() { exportPlanConfig(*document); });
            menu.addAction(QStringLiteral("Import Config (JSON)"), this, [this, document]() { importPlanConfig(*document); });
            menu.addSeparator();
            menu.addAction(QStringLiteral("Delete"), this, [this, document]() { removeDocument(*document); });
        }
        menu.exec(global_pos);
        return;
    }
    // Folder or root.
    QMenu menu(panel_);
    menu.addAction(QStringLiteral("Add Folder"), this, &ExternalDocumentsView::addFolder);
    menu.addAction(QStringLiteral("Import Document"), this, &ExternalDocumentsView::importDocuments);
    if (kind == QLatin1String("folder")) {
        const QString folder_id = item->data(0, kRoleId).toString();
        menu.addSeparator();
        menu.addAction(QStringLiteral("Rename"), this, [this, folder_id]() {
            bool ok = false;
            const QString name = QInputDialog::getText(panel_, QStringLiteral("Rename folder"),
                                                       QStringLiteral("Name:"), QLineEdit::Normal,
                                                       store_.folder(folder_id)->name, &ok);
            if (ok && !name.isEmpty()) store_.renameFolder(folder_id, name);
        });
        menu.addAction(QStringLiteral("Delete"), this, [this, folder_id]() { store_.removeFolder(folder_id); });
    }
    menu.addSeparator();
    menu.addAction(QStringLiteral("Save Documents (JSON)"), this, &ExternalDocumentsView::saveDocumentsJson);
    menu.addAction(QStringLiteral("Load Documents (JSON)"), this, &ExternalDocumentsView::loadDocumentsJson);
    menu.exec(global_pos);
}

void ExternalDocumentsView::showPlanClickMenu(ExternalDocument& document, const QPoint& global_pos) {
    QMenu menu(panel_);
    menu.addAction(QStringLiteral("Modify Plan"), this, [this, &document]() { editPlan(document); });
    menu.addAction(document.plan.visible ? QStringLiteral("Hide") : QStringLiteral("Show"),
                   this, [this, &document]() { togglePlanVisibility(document); });
    menu.addSeparator();
    menu.addAction(QStringLiteral("Delete"), this, [this, &document]() { removeDocument(document); });
    menu.exec(global_pos);
}

void ExternalDocumentsView::showRootMenu(const QPoint& global_pos) {
    showItemContextMenu(nullptr, global_pos);
}

// --- import / folders -----------------------------------------------------------

void ExternalDocumentsView::addFolder() {
    bool ok = false;
    const QString name = QInputDialog::getText(panel_, QStringLiteral("Add folder"),
                                               QStringLiteral("Folder name:"), QLineEdit::Normal,
                                               QStringLiteral("New folder"), &ok);
    if (!ok || name.isEmpty()) return;
    store_.addFolder(name, selectedFolderId());
}

void ExternalDocumentsView::importDocuments() {
    const QString folder_id = selectedFolderId();
    const QStringList paths = QFileDialog::getOpenFileNames(panel_, QStringLiteral("Import external documents"),
                                                            QString(), documentFilter());
    for (const QString& path : paths) {
        if (!isSupportedDocument(path)) continue;
        ExternalDocument& document = store_.addDocument(path, folder_id);
        loadDocument(document);
    }
    updatePlanOverlays();
    session_.setStatusMessage(QStringLiteral("External Documents"),
                              QStringLiteral("%1 document(s) imported").arg(paths.size()));
}

void ExternalDocumentsView::loadDocument(ExternalDocument& document) {
    if (QFileInfo::exists(document.path)) {
        document.state = DocumentState::Loaded;
    } else {
        document.state = DocumentState::FileNotFound;
    }
    cache_.remove(document.id);
    emit store_.documentChanged(document.id);
}

void ExternalDocumentsView::locateFile(ExternalDocument& document) {
    const QString path = QFileDialog::getOpenFileName(panel_, QStringLiteral("Locate file"), document.path,
                                                      documentFilter());
    if (path.isEmpty()) return;
    document.path = path;
    loadDocument(document);
    if (document.kind == DocumentKind::Plan) updatePlanOverlays();
    if (document.kind == DocumentKind::Table && grid_ != nullptr && grid_->documentId() == document.id) {
        visualize(document);
    }
}

void ExternalDocumentsView::removeDocument(ExternalDocument& document) {
    cache_.remove(document.id);
    if (grid_ != nullptr && grid_->documentId() == document.id) grid_->clearTable();
    store_.removeDocument(document.id);
    updatePlanOverlays();
}

// --- tables ---------------------------------------------------------------------

ExternalDocumentsView::CachedTable ExternalDocumentsView::cachedTable(ExternalDocument& document) {
    const auto cached = cache_.constFind(document.id);
    if (cached != cache_.constEnd()) return cached.value();

    CachedTable table;
    table.data = readTableFile(document.path, document.join.sheet);
    if (!table.data.ok) {
        document.state = DocumentState::FileNotFound;
        emit store_.documentChanged(document.id);
        return table;
    }
    bool key_missing = false;
    table.join = joinTable(table.data, document.join, session_, data_access_, &key_missing);
    if (key_missing) {
        document.state = DocumentState::KeyColumnMissing;
        emit store_.documentChanged(document.id);
    } else if (document.state != DocumentState::Loaded) {
        document.state = DocumentState::Loaded;
        emit store_.documentChanged(document.id);
    }
    cache_.insert(document.id, table);
    return table;
}

void ExternalDocumentsView::visualize(ExternalDocument& document) {
    CachedTable table = cachedTable(document);
    if (!table.data.ok) {
        QMessageBox::warning(panel_, QStringLiteral("External Table"), table.data.error);
        return;
    }
    ExternalGridPanel::RowInfo row_info;
    row_info.object_of_row = [join = table.join](int row) { return join.row_to_object.value(row, 0); };
    row_info.model_name_of = [this](std::uint32_t object_id) {
        if (const auto info = session_.elementRegistry()->findBasicElementInfo(object_id)) {
            return session_.modelIdForSessionModelId(info->session_model_id);
        }
        return QString();
    };
    row_info.basic_info_of = [this](std::uint32_t object_id) {
        if (const auto info = session_.elementRegistry()->findBasicElementInfo(object_id)) {
            return QStringList{info->type, info->name, info->guid};
        }
        return QStringList{};
    };
    grid_->showTable(document, table.data, table.join, row_info);
    if (table.data.total_rows > kMaxLoadedRows) {
        grid_->setCounts(QStringLiteral("%1 of %2 row(s) loaded, %3 matched")
                             .arg(table.data.rows.size()).arg(table.data.total_rows)
                             .arg(table.join.report.matched_rows));
    }
}

void ExternalDocumentsView::refresh(ExternalDocument& document) {
    cache_.remove(document.id);
    visualize(document);
}

void ExternalDocumentsView::editJoin(ExternalDocument& document) {
    CachedTable table = cachedTable(document);
    if (!table.data.ok) {
        QMessageBox::warning(panel_, QStringLiteral("External Table"), table.data.error);
        return;
    }
    JoinDialog dialog(document, table.data, session_, data_access_, catalog_, panel_);
    if (dialog.exec() != QDialog::Accepted) return;
    const JoinConfig old = document.join;
    document.join = dialog.config();
    if (old.sheet != document.join.sheet) {
        cache_.remove(document.id); // the sheet changed: re-read the file
        table = cachedTable(document);
    } else {
        bool key_missing = false;
        table.join = joinTable(table.data, document.join, session_, data_access_, &key_missing);
        document.state = key_missing ? DocumentState::KeyColumnMissing : DocumentState::Loaded;
        cache_.insert(document.id, table);
    }
    emit store_.documentChanged(document.id);
    if (grid_ != nullptr && grid_->documentId() == document.id) {
        visualize(document);
    }
}

void ExternalDocumentsView::exportCsv(ExternalDocument& document) {
    CachedTable table = cachedTable(document);
    if (!table.data.ok) return;
    const QString path = QFileDialog::getSaveFileName(panel_, QStringLiteral("Export CSV"), document.name + QStringLiteral(".csv"),
                                                      QStringLiteral("CSV (*.csv)"));
    if (path.isEmpty()) return;
    // All the rows of the file, in the displayed order when the grid is
    // sorted, else in file order.
    QList<QStringList> rows;
    if (grid_ != nullptr && grid_->documentId() == document.id && grid_->hasTable()) {
        // Sorted/filtered order would hide rows; export the file order of all
        // the loaded rows (the export writes the file, not the filtered view).
        rows = table.data.rows;
    } else {
        rows = table.data.rows;
    }
    if (!writeCsvFile(path, table.data.columns, rows)) {
        QMessageBox::warning(panel_, QStringLiteral("Export CSV"), QStringLiteral("Cannot write %1").arg(path));
    } else {
        session_.setStatusMessage(QStringLiteral("External Documents"),
                                  QStringLiteral("Exported %1 row(s) to %2").arg(rows.size()).arg(path));
    }
}

void ExternalDocumentsView::showReport(ExternalDocument& document) {
    CachedTable table = cachedTable(document);
    if (!table.data.ok) return;
    ReportDialog dialog(document.name, table.data, table.join.report, panel_);
    connect(&dialog, &ReportDialog::objectSelected, this, [this](std::uint32_t object_id) {
        selection_.setSelection({object_id}, services::SelectionSource::Table,
                                services::SelectionMode::Replace);
    });
    dialog.exec();
}

// --- plans ----------------------------------------------------------------------

void ExternalDocumentsView::togglePlanVisibility(ExternalDocument& document) {
    undo_stack_.record(); // CTRL+Z restores the previous visibility (depth 10)
    document.plan.visible = !document.plan.visible;
    emit store_.documentChanged(document.id);
    updatePlanOverlays();
}

void ExternalDocumentsView::editPlan(ExternalDocument& document) {
    PlanDialog dialog(document, panel_);
    if (dialog.exec() != QDialog::Accepted) return;
    undo_stack_.record(); // CTRL+Z restores the previous transform
    document.plan = dialog.config();
    if (dialog.sourcePath() != document.path) {
        document.path = dialog.sourcePath();
        document.name = QFileInfo(document.path).fileName();
        loadDocument(document);
    }
    emit store_.documentChanged(document.id);
    updatePlanOverlays();
}

void ExternalDocumentsView::exportPlanConfig(ExternalDocument& document) {
    const QString path = QFileDialog::getSaveFileName(panel_, QStringLiteral("Export Config (JSON)"),
                                                      document.name + QStringLiteral(".json"),
                                                      QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) return;
    // One plan configuration; the source path is relative to the JSON file
    // when possible.
    QJsonObject doc_json = document.toJson();
    QString relative = QDir(QFileInfo(path).absolutePath()).relativeFilePath(document.path);
    if (!relative.startsWith(QLatin1String(".."))) doc_json["path"] = relative;
    QJsonObject json;
    json["version"] = 1;
    json["documents"] = QJsonArray{doc_json};
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
    }
}

void ExternalDocumentsView::importPlanConfig(ExternalDocument& document) {
    const QString path = QFileDialog::getOpenFileName(panel_, QStringLiteral("Import Config (JSON)"),
                                                      QString(), QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
    for (const QJsonValue& value : json["documents"].toArray()) {
        ExternalDocument loaded = ExternalDocument::fromJson(value.toObject());
        if (loaded.kind != DocumentKind::Plan) continue;
        undo_stack_.record();
        document.plan = loaded.plan;
        document.path = resolveAgainst(loaded.path, path);
        document.name = loaded.name.isEmpty() ? QFileInfo(document.path).fileName() : loaded.name;
        loadDocument(document);
        emit store_.documentChanged(document.id);
        updatePlanOverlays();
        return;
    }
}

void ExternalDocumentsView::updatePlanOverlays() {
    std::vector<float> triangles;
    std::vector<OverlayRenderer::LineGroup> lines;
    std::vector<OverlayRenderer::Label> labels;
    float max_opacity = 0.0f;
    for (const ExternalDocument& document : store_.documents()) {
        if (document.kind != DocumentKind::Plan || !document.plan.visible) continue;
        if (document.state == DocumentState::FileNotFound) continue;
        const PlanGeometry geometry = planQuad(document.path, document.plan);
        triangles.insert(triangles.end(), geometry.quad_triangles.begin(), geometry.quad_triangles.end());
        // Outline of the quad, one segment per edge, with the plan's opacity.
        // The quad triangles hold the 4 corners as vertices 0..3 (v0,v1,v2, v0,v2,v3).
        OverlayRenderer::LineGroup group;
        const float* t = geometry.quad_triangles.data();
        const int edge_vertex[4][2] = {{0, 3}, {3, 6}, {6, 9}, {9, 0}}; // v0-v1, v1-v2, v2-v3, v3-v0
        for (const auto& edge : edge_vertex) {
            for (int k = 0; k < 2; ++k) {
                for (int c = 0; c < 3; ++c) {
                    group.world_xyz.push_back(t[edge[k] * 3 + c]);
                }
            }
        }
        const float alpha = float(std::min(1.0, std::max(0.0, document.plan.opacity)));
        group.color[0] = 0.35f; group.color[1] = 0.55f; group.color[2] = 0.95f; group.color[3] = alpha;
        group.line_width = 2.0f;
        lines.push_back(group);
        OverlayRenderer::Label label;
        label.world_pos[0] = geometry.center[0];
        label.world_pos[1] = geometry.center[1];
        label.world_pos[2] = geometry.center[2];
        label.text = document.name;
        labels.push_back(label);
        max_opacity = std::max(max_opacity, alpha);
    }
    // The filled quad goes through the translucent-triangle channel (shared
    // with the area measurement tool in phase 1); the outline and the label
    // stay on their own channels.
    viewport_.setHighlightTriangles(triangles, 0.35f, 0.55f, 0.95f, max_opacity * 0.35f);
    viewport_.setOverlayLines(lines);
    viewport_.setOverlayLabels(labels);
}

// --- JSON save / load of the tree ------------------------------------------------

void ExternalDocumentsView::saveDocumentsJson() {
    const QString path = QFileDialog::getSaveFileName(panel_, QStringLiteral("Save Documents (JSON)"),
                                                      QStringLiteral("documents.json"),
                                                      QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) return;
    // Source paths relative to the JSON file when possible.
    QList<ExternalDocument> documents = store_.documents();
    const QDir dir(QFileInfo(path).absolutePath());
    for (ExternalDocument& document : documents) {
        const QString relative = dir.relativeFilePath(document.path);
        if (!relative.startsWith(QLatin1String(".."))) {
            // Stored relative; restored against the JSON file.
            QJsonObject json = document.toJson();
            json["path"] = relative;
            document = ExternalDocument::fromJson(json);
        }
    }
    QJsonObject json = treeToJson(store_.folders(), documents);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
        session_.setStatusMessage(QStringLiteral("External Documents"),
                                  QStringLiteral("Saved %1 document(s) to %2").arg(documents.size()).arg(path));
    } else {
        QMessageBox::warning(panel_, QStringLiteral("Save Documents"), QStringLiteral("Cannot write %1").arg(path));
    }
}

void ExternalDocumentsView::loadDocumentsJson() {
    const QString path = QFileDialog::getOpenFileName(panel_, QStringLiteral("Load Documents (JSON)"),
                                                      QString(), QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
    QList<FolderNode> folders;
    QList<ExternalDocument> documents;
    treeFromJson(json, folders, documents);
    // Resolve the source paths relative to the JSON file; update the
    // documents already in the tree (matched by id, else by path), create
    // the others.
    for (ExternalDocument& document : documents) {
        document.path = resolveAgainst(document.path, path);
        ExternalDocument* existing = store_.document(document.id);
        if (existing == nullptr) {
            for (ExternalDocument& candidate : store_.documents()) {
                if (candidate.path == document.path) { existing = &candidate; break; }
            }
        }
        if (existing != nullptr) {
            existing->join = document.join;
            existing->plan = document.plan;
            existing->name = document.name.isEmpty() ? existing->name : document.name;
            existing->path = document.path;
            if (document.folder_id.isEmpty() == false) existing->folder_id = document.folder_id;
            loadDocument(*existing);
        } else {
            ExternalDocument& added = store_.addDocument(document.path, document.folder_id);
            added.id = document.id;
            added.name = document.name;
            added.kind = document.kind;
            added.join = document.join;
            added.plan = document.plan;
            loadDocument(added);
        }
    }
    for (const FolderNode& folder : folders) {
        if (store_.folder(folder.id) == nullptr) store_.addFolder(folder.name, folder.parent_id);
    }
    updatePlanOverlays();
    session_.setStatusMessage(QStringLiteral("External Documents"),
                              QStringLiteral("Loaded %1 document(s) from %2").arg(documents.size()).arg(path));
}

// --- project persistence ----------------------------------------------------------

void ExternalDocumentsView::saveToFederation() {
    Federation* federation = session_.federation();
    if (federation == nullptr) return;
    if (store_.documents().isEmpty() && store_.folders().isEmpty()) {
        federation->setExtension(kExtensionKey, QJsonValue());
        return;
    }
    federation->setExtension(kExtensionKey, store_.toJson());
}

void ExternalDocumentsView::restoreFromFederation() {
    Federation* federation = session_.federation();
    const QJsonValue section = federation != nullptr ? federation->extension(kExtensionKey) : QJsonValue();
    store_.clear();
    cache_.clear();
    if (section.isObject()) {
        store_.fromJson(section.toObject());
        for (ExternalDocument& document : store_.documents()) {
            loadDocument(document);
        }
    }
    rebuildTree();
    updatePlanOverlays();
}

} // namespace bonsaiviewer::modules::external_documents
