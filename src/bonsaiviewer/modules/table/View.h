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

#ifndef IFCINTERFACE_MODULES_TABLE_VIEW_H
#define IFCINTERFACE_MODULES_TABLE_VIEW_H

#include "Table.h"

#include <QObject>
#include <QStringList>

#include <cstdint>
#include <optional>
#include <vector>

namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::services {
class ActionPipeline;
class AppearanceService;
class EntitySetStore;
class IfcDataAccess;
class PropertyCatalog;
class SelectionService;
}

namespace bonsaiviewer::modules::table {

class TableModel;
class TablePanel;
class TableStore;

// TI07 — drives the Table panel: creates and modifies the table, evaluates
// its IDS selection on every loaded model (one IfcTester request per
// model, in sequence), edits the columns from the header menu, selects /
// isolates the elements of the clicked rows and exports to CSV. Phase 2:
// saves the table to the Tables panel and loads a saved table.
class TablePanelView : public QObject {
    Q_OBJECT

public:
    TablePanelView(TablePanel* panel,
                   SessionState& session,
                   bonsaiviewer::services::IdsBridge& bridge,
                   bonsaiviewer::services::IfcDataAccess& data_access,
                   bonsaiviewer::services::PropertyCatalog& catalog,
                   bonsaiviewer::services::SelectionService& selection,
                   bonsaiviewer::services::ActionPipeline& actions,
                   TableStore& store,
                   bonsaiviewer::services::AppearanceService& appearance,
                   bonsaiviewer::services::EntitySetStore& box,
                   QObject* parent = nullptr);

    // A new unsaved table; `folder_id` is where it is saved (empty = root).
    void newTable(const QString& folder_id = {});
    // Saves the current table in the store (a new one, or the stored one).
    void saveTable();
    // Loads a saved table and evaluates it on the loaded models. With
    // `on_box`, only the elements of the current box are kept (not saved).
    void openTable(const QString& table_id, bool on_box = false);

private:
    struct ModelSource {
        std::uint32_t session_model_id = 0;
        QString ifc_path;
    };

    void modifyTable();
    bool editTable(TableDefinition& definition);
    void addColumn(int position);
    void modifyColumn(int index);
    void removeColumn(int index);
    void showHeaderMenu(int section);
    void moveColumn(int from, int to);
    void applyColumns();
    // The Color column: one random colour per row, in the cells and in the
    // viewer (AppearanceService table layer).
    void applyRowColours();

    void evaluate();
    void evaluateNext();
    void finishEvaluation();
    void updateSummary();

    void selectRows();
    // The rows an action applies to: the selected rows, or the clicked row
    // when it is not selected.
    std::vector<int> rowsForAction(int clicked_row) const;
    void isolateRows(const std::vector<int>& rows);
    void isolateExpressIds(std::uint32_t session_model_id, const std::vector<int>& express_ids);
    void exportCsv();

    std::vector<std::uint32_t> objectIds(std::uint32_t session_model_id, const std::vector<int>& express_ids) const;
    std::uint32_t activeSessionModelId() const;
    QString ifcPath(std::uint32_t session_model_id) const;

    TablePanel* panel_ = nullptr;
    SessionState& session_;
    bonsaiviewer::services::IdsBridge& bridge_;
    bonsaiviewer::services::PropertyCatalog& catalog_;
    bonsaiviewer::services::SelectionService& selection_;
    bonsaiviewer::services::ActionPipeline& actions_;
    TableStore& store_;
    bonsaiviewer::services::AppearanceService& appearance_;
    bonsaiviewer::services::EntitySetStore& box_;
    TableModel* model_ = nullptr;

    std::optional<TableDefinition> table_;
    // The stored table shown, empty while the table is not saved; the
    // folder a new table is saved into.
    QString current_id_;
    QString pending_folder_;
    // Run on Box: the run keeps only the elements of the box. A run option,
    // not saved with the table; it follows the box while it is set.
    bool apply_to_box_ = false;

    // The evaluation in progress.
    int generation_ = 0;
    bool evaluating_ = false;
    std::vector<ModelSource> sources_;
    size_t source_index_ = 0;
    std::vector<std::uint32_t> found_;
    QStringList errors_;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_VIEW_H
