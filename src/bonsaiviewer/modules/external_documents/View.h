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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_VIEW_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_VIEW_H

#include "DocumentStore.h"
#include "JoinEngine.h"
#include "TableFile.h"

#include <QHash>
#include <QObject>

class QTreeWidget;
class QTreeWidgetItem;
class ViewportWindow;

namespace bonsaiviewer {
class SessionState;
namespace services {
class ActiveView;
class IfcDataAccess;
class PropertyCatalog;
class SelectionService;
class UndoStack;
}
} // namespace bonsaiviewer

namespace bonsaiviewer::modules::external_documents {

class ExternalDocumentsPanel;
class ExternalGridPanel;

// BonsaiViewer35/36 — the logic of the External Documents panel: populates
// the tree from the DocumentStore, runs the context menus (import, folders,
// visualize / edit join / refresh / report for tables; show-hide / edit plan
// / JSON config for plans), drives the External Table grid, draws the shown
// plans as backgrounds of the 3D view, and persists the tree + document
// configurations with the project (.ifcfed extension) and as JSON files.
class ExternalDocumentsView : public QObject {
    Q_OBJECT

public:
    ExternalDocumentsView(SessionState& session, ViewportWindow& viewport,
                          services::IfcDataAccess& data_access,
                          services::SelectionService& selection,
                          services::ActiveView& active_view,
                          services::UndoStack& undo_stack,
                          services::PropertyCatalog& catalog,
                          ExternalDocumentsPanel* panel,
                          ExternalGridPanel* grid,
                          QObject* parent = nullptr);

private:
    // Tree.
    void rebuildTree();
    QTreeWidgetItem* addFolderItem(QTreeWidgetItem* parent, const FolderNode& folder);
    void addDocumentItem(QTreeWidgetItem* parent, const ExternalDocument& document);
    QString selectedFolderId();
    ExternalDocument* selectedDocument();
    void showItemContextMenu(QTreeWidgetItem* item, const QPoint& global_pos);
    void showPlanClickMenu(ExternalDocument& document, const QPoint& global_pos);
    void showRootMenu(const QPoint& global_pos);
    void onItemClicked(QTreeWidgetItem* item);

    // Import / folders.
    void importDocuments();
    void addFolder();
    void loadDocument(ExternalDocument& document); // state from the file on disk

    // Tables (BonsaiViewer35).
    void visualize(ExternalDocument& document);
    void refresh(ExternalDocument& document);
    void editJoin(ExternalDocument& document);
    void exportCsv(ExternalDocument& document);
    void showReport(ExternalDocument& document);
    void locateFile(ExternalDocument& document);
    void removeDocument(ExternalDocument& document);

    // Plans (BonsaiViewer36).
    void togglePlanVisibility(ExternalDocument& document);
    void editPlan(ExternalDocument& document);
    void exportPlanConfig(ExternalDocument& document);
    void importPlanConfig(ExternalDocument& document);
    void updatePlanOverlays();

    // JSON save / load of the whole tree (root or folder context menu).
    void saveDocumentsJson();
    void loadDocumentsJson();

    // Project persistence (.ifcfed extension).
    void saveToFederation();
    void restoreFromFederation();

    struct CachedTable {
        TableData data;
        JoinResult join;
    };
    CachedTable cachedTable(ExternalDocument& document);

    SessionState& session_;
    ViewportWindow& viewport_;
    services::IfcDataAccess& data_access_;
    services::SelectionService& selection_;
    services::ActiveView& active_view_;
    services::UndoStack& undo_stack_;
    services::PropertyCatalog& catalog_;
    ExternalDocumentsPanel* panel_ = nullptr;
    ExternalGridPanel* grid_ = nullptr;

    DocumentStore store_;
    QHash<QString, CachedTable> cache_;
};

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_VIEW_H
