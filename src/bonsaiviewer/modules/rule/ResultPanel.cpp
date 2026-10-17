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

#include "ResultPanel.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::rule {

namespace {

constexpr int kRowIndexRole = Qt::UserRole + 10;   // result index of a row item
constexpr int kIsGroupRole = Qt::UserRole + 11;   // 1 = grouping node

// The result index stored on a row item; -1 on group nodes.
int rowIndexOf(const QTreeWidgetItem* item) {
    const QVariant value = item->data(0, kRowIndexRole);
    return value.isValid() ? value.toInt() : -1;
}
constexpr int kColumnSource = 0;
constexpr int kColumnTarget = 1;
constexpr int kColumnModel = 2;
constexpr int kColumnRule = 3;
constexpr int kColumnMeasure = 4;
constexpr int kColumnValid = 5;
constexpr int kColumnNonDetermined = 6;
constexpr int kColumnInvalid = 7;

void setChecks(QTreeWidgetItem* item, const ResultRow& row) {
    item->setCheckState(kColumnValid,
        row.status == ResultStatus::Valid ? Qt::Checked : Qt::Unchecked);
    item->setCheckState(kColumnNonDetermined,
        row.status == ResultStatus::NonDetermined ? Qt::Checked : Qt::Unchecked);
    item->setCheckState(kColumnInvalid,
        row.status == ResultStatus::Invalid ? Qt::Checked : Qt::Unchecked);
}

QString modelText(const ResultRow& row) {
    if (row.source_model_name == row.target_model_name) return row.source_model_name;
    return QString("%1 → %2").arg(row.source_model_name, row.target_model_name);
}

} // namespace

ResultPanel::ResultPanel(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    outer->setSpacing(4);

    // SPEC Clash Filter 3: a text bar filters the results by the name of
    // the clashing elements.
    filter_edit_ = new QLineEdit(this);
    filter_edit_->setPlaceholderText("Filter results by element name");
    connect(filter_edit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        name_filter_ = text;
        rebuildView();
    });
    outer->addWidget(filter_edit_);

    tree_ = new QTreeWidget(this);
    tree_->setColumnCount(8);
    tree_->setHeaderLabels({"Source", "Target", "Model", "Rule", "Measure", "Valide", "Non Déterminé", "Invalide"});
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree_->setContextMenuPolicy(Qt::DefaultContextMenu);
    tree_->header()->setStretchLastSection(false);
    tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tree_->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    outer->addWidget(tree_, 1);

    // Clicking a status column checks it and clears the two others; the
    // current row drives the Result Info panel.
    connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int column) {
        const int row_index = rowIndexOf(item);
        if (row_index >= 0 && column >= kColumnValid && column <= kColumnInvalid) {
            switch (column) {
            case kColumnValid: applyStatus(row_index, ResultStatus::Valid); break;
            case kColumnNonDetermined: applyStatus(row_index, ResultStatus::NonDetermined); break;
            case kColumnInvalid: applyStatus(row_index, ResultStatus::Invalid); break;
            }
            return;
        }
        if (row_index >= 0) emit currentResultChanged(row_index);
    });
    // Double-click on a result row shows and zooms to the pair.
    connect(tree_, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* item, int column) {
        (void)column;
        const int row_index = rowIndexOf(item);
        if (row_index >= 0) emit resultShowRequested(row_index);
    });
}

void ResultPanel::setResults(const std::vector<ResultRow>& rows) {
    rows_ = rows;
    rebuildView();
}

void ResultPanel::setGroupMode(GroupMode mode) {
    // SPEC Clash Filter 2: grouping by target is a no-op when no row has
    // one (group by source likewise relies on the source side).
    if (mode == GroupMode::ByTarget) {
        bool any = false;
        for (const ResultRow& row : rows_) {
            if (!row.target_name.isEmpty()) { any = true; break; }
        }
        if (!any) return;
    }
    group_mode_ = mode;
    rebuildView();
}

void ResultPanel::setFilterOnlyNonDetermined(bool on) {
    only_non_determined_ = on;
    rebuildView();
}

QTreeWidgetItem* ResultPanel::appendRow(QTreeWidgetItem* parent, int row_index) {
    const ResultRow& row = rows_[row_index];
    auto* item = new QTreeWidgetItem(parent);
    item->setText(kColumnSource, row.source_name);
    item->setText(kColumnTarget, row.target_name);
    item->setText(kColumnModel, modelText(row));
    item->setText(kColumnRule, row.rule_type);
    item->setText(kColumnMeasure, row.measure);
    item->setData(0, kRowIndexRole, row_index);
    item->setData(0, kIsGroupRole, 0);
    setChecks(item, row);
    if (parent) parent->addChild(item);
    else tree_->addTopLevelItem(item);
    return item;
}

void ResultPanel::rebuildView() {
    tree_->clear();

    auto passes = [this](int row_index) {
        const ResultRow& row = rows_[row_index];
        if (only_non_determined_ && row.status != ResultStatus::NonDetermined) return false;
        if (!name_filter_.isEmpty()) {
            if (!row.source_name.contains(name_filter_, Qt::CaseInsensitive) &&
                !row.target_name.contains(name_filter_, Qt::CaseInsensitive)) {
                return false;
            }
        }
        return true;
    };

    if (group_mode_ == GroupMode::Flat) {
        for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
            if (passes(i)) appendRow(nullptr, i);
        }
        return;
    }

    // Grouped mode: one node per entity of the chosen side, its label
    // carrying the group size.
    const bool by_source = group_mode_ == GroupMode::BySource;
    std::vector<QString> group_order;
    std::unordered_map<QString, std::vector<int>> groups;
    for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
        const QString key = by_source ? rows_[i].source_name : rows_[i].target_name;
        if (!passes(i)) continue;
        if (key.isEmpty()) continue;
        if (!groups.count(key)) group_order.push_back(key);
        groups[key].push_back(i);
    }
    for (const QString& key : group_order) {
        auto* group_item = new QTreeWidgetItem(tree_);
        group_item->setText(kColumnSource, key);
        group_item->setText(kColumnModel, QString("(%1)").arg(groups[key].size()));
        group_item->setData(0, kIsGroupRole, 1);
        group_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        tree_->addTopLevelItem(group_item);
        for (int row_index : groups[key]) appendRow(group_item, row_index);
        group_item->setExpanded(true);
    }
}

void ResultPanel::applyStatus(int row_index, ResultStatus status) {
    if (row_index < 0 || row_index >= static_cast<int>(rows_.size())) return;
    rows_[row_index].status = status;
    for (QTreeWidgetItemIterator it(tree_); *it; ++it) {
        QTreeWidgetItem* item = *it;
        if (rowIndexOf(item) == row_index) setChecks(item, rows_[row_index]);
    }
}

std::vector<int> ResultPanel::rowIndexesOfSelection() const {
    std::vector<int> indexes;
    for (QTreeWidgetItem* selected : tree_->selectedItems()) {
        // A group node spreads the action to all its rows.
        if (selected->data(0, kIsGroupRole).toInt() == 1) {
            for (int i = 0; i < selected->childCount(); ++i) {
                const int row_index = rowIndexOf(selected->child(i));
                if (row_index >= 0) indexes.push_back(row_index);
            }
        } else {
            const int row_index = rowIndexOf(selected);
            if (row_index >= 0) indexes.push_back(row_index);
        }
    }
    std::sort(indexes.begin(), indexes.end());
    indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());
    return indexes;
}

std::vector<uint32_t> ResultPanel::objectIdsOfSelection() const {
    std::vector<uint32_t> object_ids;
    for (int row_index : rowIndexesOfSelection()) {
        if (rows_[row_index].source_object_id) object_ids.push_back(rows_[row_index].source_object_id);
        if (rows_[row_index].target_object_id) object_ids.push_back(rows_[row_index].target_object_id);
    }
    return object_ids;
}

void ResultPanel::contextMenuEvent(QContextMenuEvent* event) {
    const std::vector<int> selection = rowIndexesOfSelection();
    if (selection.empty()) return;

    QMenu menu(this);
    QAction* show = menu.addAction("Afficher les objets");
    connect(show, &QAction::triggered, this, [this, selection]() {
        (void)selection;
        emit objectsShowRequested(objectIdsOfSelection());
    });
    menu.addSeparator();
    QAction* valid = menu.addAction("Validé les results");
    connect(valid, &QAction::triggered, this, [this, selection]() {
        for (int row_index : selection) applyStatus(row_index, ResultStatus::Valid);
    });
    QAction* non_determined = menu.addAction("Non déterminé les résults");
    connect(non_determined, &QAction::triggered, this, [this, selection]() {
        for (int row_index : selection) applyStatus(row_index, ResultStatus::NonDetermined);
    });
    QAction* invalid = menu.addAction("Invalidé les results");
    connect(invalid, &QAction::triggered, this, [this, selection]() {
        for (int row_index : selection) applyStatus(row_index, ResultStatus::Invalid);
    });
    menu.exec(event->globalPos());
}

} // namespace bonsaiviewer::modules::rule
