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

#ifndef IFCINTERFACE_MODULES_RULE_RESULTS_H
#define IFCINTERFACE_MODULES_RULE_RESULTS_H

#include "core/Result.h"

#include <QString>
#include <QStringList>
#include <QWidget>

#include <cstdint>
#include <memory>
#include <vector>

class QCheckBox;
class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;

namespace bonsaiviewer::modules::rule {

// The review of a result, decided in the Result panel (session only).
enum class ReviewStatus { Valid, NonDetermined, Invalid };

// One result of the last run, resolved to the viewer's models and objects.
struct ResultRow {
    QString node_id;
    QString rule_label;
    QString rule_type;
    QString source_name, target_name;
    QString source_guid, target_guid;
    QString source_class, target_class;
    QString source_model_name, target_model_name;
    std::uint32_t source_model = 0; // session model ids
    std::uint32_t target_model = 0;
    int source_ifc_id = 0;
    int target_ifc_id = 0;
    std::vector<std::uint32_t> source_objects; // viewer object ids (several for a Must group)
    std::vector<std::uint32_t> target_objects;
    QString measure;
    QStringList criticity;
    QStringList actor;
    QString group;      // the rule's grouping / Must group
    bool status = true; // false: excepted (or a failing Must group)
    bool complex = false;
    ReviewStatus review = ReviewStatus::NonDetermined;
    std::shared_ptr<const ifcclash::ResultRecord> record;

    std::vector<std::uint32_t> objects() const;
};

// BonsaiViewer09 (TI09) — the Result panel of the Clash Filter workspace
// (branch IfcViewer_Clash): one row per result, a name filter, grouping
// (by source, target, rule, criticity, rule group), the review status
// (Valid / Non Determined / Invalid). Click: select the pair; double
// click: isolate it; right click: the standard menu.
class ResultPanel : public QWidget {
    Q_OBJECT

public:
    enum class GroupMode { Flat, BySource, ByTarget, ByRule, ByCriticity, ByRuleGroup };

    explicit ResultPanel(QWidget* parent = nullptr);

    void setResults(std::vector<ResultRow> rows);
    // The warnings of the run (elements the engine could not test, folders
    // left out, failed rules): a line above the table, the list as tooltip.
    void setMessages(const QStringList& messages);
    const std::vector<ResultRow>& rows() const { return rows_; }
    void setGroupMode(GroupMode mode);
    void setFilterOnlyNonDetermined(bool on);
    void setHideExcepted(bool on);
    void clearFilters();
    // The rows shown, in order (the filters applied).
    std::vector<int> shownRows() const;
    std::vector<int> selectedRows() const;

signals:
    void currentResultChanged(int row);
    void rowsSelected(const std::vector<int>& rows);
    void isolateRequested(const std::vector<int>& rows);
    void showRequested(const std::vector<int>& rows);
    void reviewChanged();

private:
    void rebuild();
    QTreeWidgetItem* appendRow(QTreeWidgetItem* parent, int row);
    void setReview(const std::vector<int>& rows, ReviewStatus status);
    void contextMenu(const QPoint& pos);

    std::vector<ResultRow> rows_;
    GroupMode mode_ = GroupMode::Flat;
    bool only_non_determined_ = false;
    bool hide_excepted_ = false;
    QString filter_;
    QLineEdit* filter_edit_ = nullptr;
    QLabel* messages_ = nullptr;
    QTreeWidget* tree_ = nullptr;
};

// Result Info: the details of the current result — the elements, the
// measures, the labels, and the geometry blocks the rule gave (points,
// faces, boxes, axes, rays; drawn from the RESULT GEOMETRY ribbon group).
class ResultInfoPanel : public QWidget {
    Q_OBJECT

public:
    explicit ResultInfoPanel(QWidget* parent = nullptr);
    void showRow(const ResultRow* row);

private:
    QLabel* text_ = nullptr;
};

QString measureText(const ifcclash::ResultRecord& record);
QString measureUnit(ifcclash::Measure measure);

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_RESULTS_H
