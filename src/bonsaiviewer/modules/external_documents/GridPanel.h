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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_GRIDPANEL_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_GRIDPANEL_H

#include "../../components/Panel.h"

#include "JoinEngine.h"
#include "TableFile.h"
#include "Types.h"

#include <QSet>
#include <QStringList>

#include <cstdint>
#include <functional>

class QHeaderView;
class QTableView;
class QAbstractTableModel;
class QSortFilterProxyModel;
class QToolButton;
class QLabel;

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer35 — the External Table grid, docked at the bottom (same place
// as the Table panel of BonsaiViewer07). Shows all the columns of the file,
// plus the model columns Model / Class / Name / GlobalId of the joined
// object at the left. Sort and filter are the ones of the Table panel
// phase 3: an arrow and a funnel on each column header. A row whose key
// matches no loaded object is greyed and selects nothing.
class ExternalGridPanel : public components::Panel {
    Q_OBJECT

public:
    explicit ExternalGridPanel(QWidget* parent = nullptr);

    // Per source row: the joined object id, its model name, and its
    // Class / Name / GlobalId for the model columns.
    struct RowInfo {
        std::function<std::uint32_t(int)> object_of_row;      // row -> object id (0 when unmatched)
        std::function<QString(std::uint32_t)> model_name_of;  // object id -> model display name
        std::function<QStringList(std::uint32_t)> basic_info_of; // object id -> {Class, Name, GlobalId}
    };

    // Shows one document's grid. `document_name` is displayed next to the counts.
    void showTable(const ExternalDocument& document, const TableData& data,
                   const JoinResult& join, const RowInfo& row_info);
    void clearTable();
    bool hasTable() const { return model_ != nullptr; }
    QString documentId() const { return document_id_; }

    // The counts shown in the toolbar ("12 of 40 row(s)", matched counts).
    void setCounts(const QString& text);

signals:
    void refreshRequested();
    void editJoinRequested();
    void exportCsvRequested();
    void showReportRequested();
    // Click on a row: the joined object(s) of the selected rows.
    void rowsSelected(const QList<std::uint32_t>& ids, bool add_to_selection);
    void rowsIsolateRequested(const QList<std::uint32_t>& ids);

private:
    void buildFilterMenu(int column, const QPoint& global_pos);
    QList<std::uint32_t> selectedRowObjectIds() const;

    QTableView* view_ = nullptr;
    QAbstractTableModel* model_ = nullptr;   // owned
    QSortFilterProxyModel* proxy_ = nullptr; // owned
    QHeaderView* header_ = nullptr;          // owned
    QToolButton* refresh_button_ = nullptr;
    QToolButton* edit_join_button_ = nullptr;
    QToolButton* export_button_ = nullptr;
    QToolButton* report_button_ = nullptr;
    QLabel* counts_label_ = nullptr;
    QLabel* title_label_ = nullptr;

    QString document_id_;
    TableData data_;
    JoinResult join_;
    RowInfo row_info_;

    friend class ExternalGridModel;
    friend class ExternalGridProxy;
    friend class ExternalGridHeader;
};

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_GRIDPANEL_H
