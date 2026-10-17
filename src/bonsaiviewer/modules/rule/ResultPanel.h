// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                           *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License           *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_RULE_RESULT_PANEL_H
#define IFCINTERFACE_MODULES_RULE_RESULT_PANEL_H

#include <QString>
#include <QWidget>
#include <cstdint>
#include <vector>

class QContextMenuEvent;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace bonsaiviewer::modules::rule {

// A clash pair's verdict, decided per result in the Result panel.
enum class ResultStatus {
    Valid,
    NonDetermined,   // the neutral default
    Invalid,         // a false positive
};

// One failing pair of the last rule run, resolved to viewport object ids.
struct ResultRow {
    QString rule_name;
    QString rule_type;
    QString source_name;
    QString target_name;
    QString source_model_name;
    QString target_model_name;
    uint32_t source_session_model_id = 0;
    uint32_t target_session_model_id = 0;
    int source_ifc_id = 0;
    int target_ifc_id = 0;
    uint32_t source_object_id = 0;
    uint32_t target_object_id = 0;
    // The nominal measure of the result ("distance = 0.42"), the criticity
    // and actor labels, and the engine's raw info blocks.
    QString measure;
    QString criticity;
    QString actor;
    ResultStatus status = ResultStatus::NonDetermined;
};

// The "Result" panel of the Clash Filter workspace: the results of the rule
// double-clicked in the Rule panel, with a name filter, a source/target
// grouping mode, per-row Valide / Non Déterminé / Invalide checkboxes and
// a right-click menu acting on the whole selection.
class ResultPanel : public QWidget {
    Q_OBJECT
public:
    enum class GroupMode {
        Flat,
        BySource,
        ByTarget,
    };

    explicit ResultPanel(QWidget* parent = nullptr);

    // Replaces the previous run's rows and rebuilds the view.
    void setResults(const std::vector<ResultRow>& rows);

    // Flat shows every result at top level; BySource/ByTarget group the
    // rows under one node per entity (a no-op when no row has that side).
    void setGroupMode(GroupMode mode);

    // The ribbon's "Filter Only Non Determined" toggle.
    void setFilterOnlyNonDetermined(bool on);

signals:
    // Double-click on a result row: show the pair in the viewport and zoom.
    void resultShowRequested(int row_index);
    // The current result changed (-1 = none); drives the Result Info panel.
    void currentResultChanged(int row_index);
    // "Afficher les objets" on a selection of rows / groups.
    void objectsShowRequested(const std::vector<uint32_t>& object_ids);

private:
    void rebuildView();
    QTreeWidgetItem* appendRow(QTreeWidgetItem* parent, int row_index);
    void applyStatus(int row_index, ResultStatus status);
    std::vector<uint32_t> objectIdsOfSelection() const;
    std::vector<int> rowIndexesOfSelection() const;
    void contextMenuEvent(QContextMenuEvent* event) override;

    std::vector<ResultRow> rows_;
    GroupMode group_mode_ = GroupMode::Flat;
    bool only_non_determined_ = false;
    QString name_filter_;

    QLineEdit* filter_edit_ = nullptr;
    QTreeWidget* tree_ = nullptr;
};

} // namespace bonsaiviewer::modules::rule

#endif
