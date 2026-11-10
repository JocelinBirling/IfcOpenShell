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

#ifndef IFCINTERFACE_MODULES_DIFF_REPORTDIALOG_H
#define IFCINTERFACE_MODULES_DIFF_REPORTDIALOG_H

#include "../../../ifcparse/express.h"

#include <QDialog>
#include <QString>

#include <cstdint>
#include <vector>

class QLineEdit;
class QSortFilterProxyModel;
class QStandardItemModel;
class QTableView;
class QTableWidget;

namespace bonsaiviewer::modules::diff {

// One row of the report: an element, or one change of an element.
struct ReportRow {
    QString model; // "Before", "After", "Before + After"
    QString global_id;
    QString ifc_class;
    QString name;
    QString changes; // "added", "deleted", or the change kinds
    QString group;   // property set, "Attributes", "Geometry", "Relationships"
    QString property;
    QString old_value;
    QString new_value;
    QString kind;
    std::vector<std::uint32_t> object_ids; // in the viewer, both models
};

// BonsaiViewer34 — Show Report: every change in a table (multiple
// elements). A click selects the element in both models, a double click
// isolates it, the context menu compares it; the table can be filtered,
// sorted and exported to CSV like the Table panel. A window of its own
// (not modal): the viewer stays usable while it is open.
class ReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReportDialog(QWidget* parent = nullptr);

    void setRows(std::vector<ReportRow> rows);

signals:
    void selectRequested(const std::vector<std::uint32_t>& object_ids);
    void isolateRequested(const std::vector<std::uint32_t>& object_ids);
    void compareRequested(const QString& global_id);

private:
    const ReportRow* rowAt(int displayed_row) const;
    void exportCsv();

    std::vector<ReportRow> rows_;
    QStandardItemModel* model_ = nullptr;
    QSortFilterProxyModel* proxy_ = nullptr;
    QTableView* view_ = nullptr;
    QLineEdit* filter_ = nullptr;
};

// BonsaiViewer34 — Report table for a single element: Before and After side
// by side, one row per field (model, GlobalId, class, attributes, then every
// property), the differing rows coloured.
class CompareDialog : public QDialog {
    Q_OBJECT

public:
    CompareDialog(const QString& before_model, const express::base& before,
                  const QString& after_model, const express::base& after,
                  QWidget* parent = nullptr);

private:
    QTableWidget* table_ = nullptr;
};

} // namespace bonsaiviewer::modules::diff

#endif // IFCINTERFACE_MODULES_DIFF_REPORTDIALOG_H
