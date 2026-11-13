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

#include "ResultKey.h"

#include "core/Result.h"

#include <QDialog>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

class QCheckBox;
class QComboBox;
class QToolButton;
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
    QString manual_group; // the user's own group (triage), empty: none
    bool status = true; // false: excepted (or a failing Must group)
    bool complex = false;
    ReviewStatus review = ReviewStatus::NonDetermined;
    std::shared_ptr<const ifcclash::ResultRecord> record;
    // The elements as saved (one each for a pair, every element of a Must
    // group), and the key of the result (resultKey): the review, the manual
    // group and the topic follow it from one run to the next.
    std::vector<ResultElement> sources, targets;
    QString key;
    QString topic_guid; // the BCF topic the result belongs to (empty: none)
    int number = 0;     // the short number shown to the users (resultNumberText), 0: none

    std::vector<std::uint32_t> objects() const;
};

// BonsaiViewer09 (TI09) — the Result panel of the Clash Filter workspace
// (branch IfcViewer_Clash): one row per result, a name filter, grouping
// (by source, target, source model, rule, criticity, rule group), the review
// (a drop-down list: Valid / Non Determined / Invalid). Click: select the pair; double
// click: isolate it; right click: the standard menu.
class ResultPanel : public QWidget {
    Q_OBJECT

public:
    enum class GroupMode { Flat, BySource, ByTarget, BySourceModel, ByRule, ByCriticity, ByRuleGroup, ByManualGroup, ByTopic };
    // The categories the user hides results by (Categories window, right click).
    enum class Category { Rule, SourceClass, TargetClass, SourceModel, TargetModel, Criticity, Actor, Review, ManualGroup, Topic };
    static const std::vector<Category>& categories();
    static QString categoryName(Category category);
    // The values of a row in a category ("(none)" when it has none).
    QStringList valuesOf(const ResultRow& row, Category category) const;
    // Category -> the values hidden: a row is hidden when one of its values is.
    using HiddenValues = std::map<Category, std::set<QString>>;

    explicit ResultPanel(QWidget* parent = nullptr);

    // The BCF topics the results point at (the BCF panel's file). A result
    // is linked when it has a topic that is not deleted; a topic missing
    // from the loaded file (or with no file loaded) still counts.
    struct TopicInfo {
        QString title;
        QString status;
        bool deleted = false;
    };
    void setTopics(QHash<QString, TopicInfo> topics);
    bool isLinked(const ResultRow& row) const;
    QString topicTitle(const QString& guid) const;
    // Sets (or clears, with an empty guid) the topic of rows.
    void setTopicOf(const std::vector<int>& rows, const QString& guid);

    void setResults(std::vector<ResultRow> rows);
    // A run: the rows of the rules in `ran` are replaced by `rows` (the
    // results of the other rules are kept, with their review and group). A
    // new row with the key of a replaced one keeps its review and group.
    void mergeResults(std::vector<ResultRow> rows, const std::set<QString>& ran);
    // The set-aside list (Must verdicts, excepted pairs): same lifecycle as
    // the results, but shown only in set-aside mode and never numbered.
    void mergeSetAsideResults(std::vector<ResultRow> rows, const std::set<QString>& ran);
    // Set-aside mode: the panel shows the set-aside list instead of the
    // results (the Rule Result Tools "Set aside" button).
    void setSetAsideMode(bool on);
    bool setAsideMode() const { return aside_mode_; }
    // The rules the panel shows (node ids; none: every rule), `label` above the table.
    void setScope(std::optional<std::set<QString>> node_ids, const QString& label);
    // Whether the rule ran (it may have no result).
    bool hasRun(const QString& node_id) const { return run_nodes_.count(node_id) > 0; }
    // The warnings of a run (elements the engine could not test, folders
    // left out, failed rules), each with the node it belongs to (empty: the
    // whole run): a line above the table, the list as tooltip. Only the
    // warnings of the rules in scope are shown. A run replaces the warnings
    // of the nodes in `ran` and of the run itself; the others are kept.
    using Message = std::pair<QString, QString>; // node id, text
    void mergeMessages(const std::vector<Message>& messages, const std::set<QString>& ran);
    void clearMessages();
    const std::vector<Message>& messages() const { return run_messages_; }
    // The rows of the list currently shown (set-aside mode: the set-aside list).
    const std::vector<ResultRow>& rows() const { return activeRows(); }
    // The two lists, whatever the mode (save, numbering: the main one only).
    const std::vector<ResultRow>& mainRows() const { return rows_; }
    const std::vector<ResultRow>& asideRows() const { return aside_rows_; }
    // Changes every row in place (the viewer ids after a model change), the
    // scope, the filters and the selection kept.
    void updateRows(const std::function<void(ResultRow&)>& update);
    // Changes every row of the main list only, whatever the mode (numbering).
    void updateMainRows(const std::function<void(ResultRow&)>& update);
    void setGroupMode(GroupMode mode);
    void setFilterOnlyNonDetermined(bool on);
    void setHideExcepted(bool on);
    // Hides the results linked to a BCF topic.
    void setHideLinked(bool on);
    void clearFilters();
    void setHiddenValues(HiddenValues hidden);
    const HiddenValues& hiddenValues() const { return hidden_; }
    // The user's groups, in the order of the rows.
    QStringList manualGroups() const;
    // The rows shown, in order (the filters applied).
    std::vector<int> shownRows() const;
    std::vector<int> selectedRows() const;
    // Selects and shows one result (from the Clash section of the Properties panel).
    void selectRow(int row);
    void setReviewOf(const std::vector<int>& rows, ReviewStatus status) { setReview(rows, status); }

signals:
    void currentResultChanged(int row);
    void rowsSelected(const std::vector<int>& rows);
    void isolateRequested(const std::vector<int>& rows);
    // Right click > Create a BCF Topic (one for all, or one per result).
    void createTopicsRequested(const std::vector<int>& rows, bool per_result);
    // Right click > Add to Topic (empty guid: "Other topic…"), Open the
    // Topic, Unlink from the Topic.
    void addToTopicRequested(const std::vector<int>& rows, const QString& guid);
    void openTopicRequested(const QString& guid);
    void unlinkTopicRequested(const std::vector<int>& rows);
    void showRequested(const std::vector<int>& rows);
    void reviewChanged();

private:
    std::vector<ResultRow>& activeRows() { return aside_mode_ ? aside_rows_ : rows_; }
    const std::vector<ResultRow>& activeRows() const { return aside_mode_ ? aside_rows_ : rows_; }
    void rebuild();
    QTreeWidgetItem* appendRow(QTreeWidgetItem* parent, int row);
    // The review as the style of a row: Valid bold, Invalid struck out and
    // grey, excepted grey, a failing Must group red.
    void styleRow(QTreeWidgetItem* item, const ResultRow& row) const;
    void setReview(const std::vector<int>& rows, ReviewStatus status);
    void setManualGroup(const std::vector<int>& rows, const QString& name);
    void renameManualGroup(const QString& from, const QString& to);
    void contextMenu(const QPoint& pos);
    void refreshMessages();

    std::vector<ResultRow> rows_;
    std::vector<ResultRow> aside_rows_;
    bool aside_mode_ = false;
    QHash<QString, TopicInfo> topics_;
    GroupMode mode_ = GroupMode::Flat;
    bool only_non_determined_ = false;
    bool hide_excepted_ = false;
    bool hide_linked_ = false;
    HiddenValues hidden_;
    std::optional<std::set<QString>> scope_;
    std::set<QString> run_nodes_;
    QLabel* scope_label_ = nullptr;
    QString filter_;
    QLineEdit* filter_edit_ = nullptr;
    QLabel* messages_ = nullptr;
    std::vector<Message> run_messages_;
    QTreeWidget* tree_ = nullptr;
};

// "Categories": the values of each category of the results, a check box
// each (unchecked: hidden), with the number of results.
class CategoryFilterDialog : public QDialog {
    Q_OBJECT

public:
    CategoryFilterDialog(const ResultPanel& panel, QWidget* parent = nullptr);
    ResultPanel::HiddenValues hidden() const;

private:
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
    // The title of a BCF topic, for the "BCF topic" line.
    void setTopicTitle(std::function<QString(const QString&)> title) { topic_title_ = std::move(title); }

signals:
    // "Select" by the source or the target: its objects (the Properties
    // panel follows the selection).
    void selectRequested(const std::vector<std::uint32_t>& objects);
    // The Review list.
    void reviewRequested(ReviewStatus status);

private:
    QLabel* title_ = nullptr;
    QLabel* source_ = nullptr;
    QLabel* target_ = nullptr;
    QToolButton* select_source_ = nullptr;
    QToolButton* select_target_ = nullptr;
    QWidget* target_row_ = nullptr;
    QWidget* review_row_ = nullptr;
    QComboBox* review_ = nullptr;
    QLabel* text_ = nullptr;
    std::vector<std::uint32_t> source_objects_, target_objects_;
    std::function<QString(const QString&)> topic_title_;
};

QString measureText(const ifcclash::ResultRecord& record);
QString measureUnit(ifcclash::Measure measure);

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_RESULTS_H
