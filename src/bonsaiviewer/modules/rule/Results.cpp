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

#include "Results.h"

#include "../../components/SvgIcon.h"

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include <algorithm>
#include <map>

namespace bonsaiviewer::modules::rule {

namespace {

constexpr int kRowRole = Qt::UserRole + 10;
constexpr int kGroupRole = Qt::UserRole + 11;

// Two columns: the source and the target; every other detail is in Result Info.
enum Column { kSource, kTarget, kColumns };
constexpr int kGroupNameRole = Qt::UserRole + 12; // a group node: its name

const QString kNone = QStringLiteral("(none)");

int rowOf(const QTreeWidgetItem* item) {
    const QVariant v = item->data(0, kRowRole);
    return v.isValid() ? v.toInt() : -1;
}

// The review list of a row: Valid, Non Determined, Invalid (in this order).
int reviewIndex(ReviewStatus status) {
    return status == ReviewStatus::Valid ? 0 : status == ReviewStatus::Invalid ? 2 : 1;
}
ReviewStatus reviewAt(int index) {
    return index == 0 ? ReviewStatus::Valid : index == 2 ? ReviewStatus::Invalid : ReviewStatus::NonDetermined;
}

QString reviewText(ReviewStatus status) {
    return status == ReviewStatus::Valid ? QStringLiteral("Valid")
           : status == ReviewStatus::Invalid ? QStringLiteral("Invalid")
                                             : QStringLiteral("Non Determined");
}


QString formatPoint(const ifcclash::Point3& p) {
    return QString("(%1, %2, %3)").arg(p[0], 0, 'f', 3).arg(p[1], 0, 'f', 3).arg(p[2], 0, 'f', 3);
}

} // namespace

std::vector<std::uint32_t> ResultRow::objects() const {
    std::vector<std::uint32_t> ids = source_objects;
    ids.insert(ids.end(), target_objects.begin(), target_objects.end());
    return ids;
}

QString measureUnit(ifcclash::Measure measure) {
    switch (measure) {
    case ifcclash::Measure::Distance:
    case ifcclash::Measure::PenetrationDepth:
    case ifcclash::Measure::Offset: return QStringLiteral("m");
    case ifcclash::Measure::ContactArea:
    case ifcclash::Measure::Area: return QStringLiteral("m²");
    case ifcclash::Measure::Volume: return QStringLiteral("m³");
    case ifcclash::Measure::Angle: return QStringLiteral("°");
    case ifcclash::Measure::Ratio:
    case ifcclash::Measure::HitRatio: return QString();
    }
    return {};
}

QString measureText(const ifcclash::ResultRecord& record) {
    if (record.measures.empty()) return {};
    const auto& [measure, value] = record.measures.items().front();
    const QString unit = measureUnit(measure);
    return QString("%1 = %2%3").arg(QString::fromLatin1(ifcclash::measure_name(measure)))
        .arg(value, 0, 'g', 4)
        .arg(unit.isEmpty() ? QString() : " " + unit);
}

// --- ResultPanel ---------------------------------------------------------------------------

ResultPanel::ResultPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    filter_edit_ = new QLineEdit(this);
    filter_edit_->setPlaceholderText("Filter the results (element name, rule, class)");
    filter_edit_->setClearButtonEnabled(true);
    connect(filter_edit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        filter_ = text;
        rebuild();
    });
    layout->addWidget(filter_edit_);
    messages_ = new QLabel(this);
    messages_->setWordWrap(true);
    messages_->setVisible(false);
    messages_->setStyleSheet("color: #B8860B;");
    layout->addWidget(messages_);
    scope_label_ = new QLabel(this);
    scope_label_->setWordWrap(true);
    scope_label_->setVisible(false);
    scope_label_->setProperty("textRole", "secondary");
    layout->addWidget(scope_label_);
    tree_ = new QTreeWidget(this);
    tree_->setColumnCount(kColumns);
    tree_->setHeaderLabels({"Source", "Target"});
    tree_->headerItem()->setToolTip(kSource, "The rule, the measure, the review, the criticity, the actor… of the "
                                             "selected result: in Result Info.");
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->setRootIsDecorated(true);
    tree_->header()->setSectionResizeMode(kSource, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(kTarget, QHeaderView::Stretch);
    layout->addWidget(tree_, 1);
    connect(tree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item, QTreeWidgetItem*) {
        emit currentResultChanged(item ? rowOf(item) : -1);
    });
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, [this]() { emit rowsSelected(selectedRows()); });
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        const int row = rowOf(item);
        if (row >= 0) emit isolateRequested({row});
        else emit isolateRequested(selectedRows());
    });
    connect(tree_, &QWidget::customContextMenuRequested, this, &ResultPanel::contextMenu);
}

void ResultPanel::setResults(std::vector<ResultRow> rows) {
    rows_ = std::move(rows);
    run_nodes_.clear();
    for (const ResultRow& row : rows_) run_nodes_.insert(row.node_id);
    scope_.reset();
    scope_label_->setVisible(false);
    rebuild();
}

void ResultPanel::mergeResults(std::vector<ResultRow> rows, const std::set<QString>& ran) {
    rows_.erase(std::remove_if(rows_.begin(), rows_.end(), [&ran](const ResultRow& row) { return ran.count(row.node_id) > 0; }),
                rows_.end());
    for (ResultRow& row : rows) rows_.push_back(std::move(row));
    run_nodes_.insert(ran.begin(), ran.end());
    rebuild();
}

void ResultPanel::setScope(std::optional<std::set<QString>> node_ids, const QString& label) {
    scope_ = std::move(node_ids);
    scope_label_->setText(label);
    scope_label_->setVisible(scope_.has_value() && !label.isEmpty());
    rebuild();
}

void ResultPanel::setMessages(const QStringList& messages) {
    messages_->setVisible(!messages.isEmpty());
    if (messages.isEmpty()) return;
    messages_->setText(QString("⚠ %1 warning%2 — %3%4")
                           .arg(messages.size())
                           .arg(messages.size() == 1 ? "" : "s")
                           .arg(messages.front().toHtmlEscaped())
                           .arg(messages.size() > 1 ? " (hover for all)" : ""));
    QStringList escaped;
    for (const QString& message : messages.mid(0, 40)) escaped << message.toHtmlEscaped();
    if (messages.size() > 40) escaped << "…";
    messages_->setToolTip("<b>Run warnings</b><br>" + escaped.join("<br>"));
}

const std::vector<ResultPanel::Category>& ResultPanel::categories() {
    static const std::vector<Category> all{Category::Rule,        Category::SourceClass, Category::TargetClass,
                                           Category::SourceModel, Category::TargetModel, Category::Criticity,
                                           Category::Actor,       Category::Review,      Category::ManualGroup};
    return all;
}

QString ResultPanel::categoryName(Category category) {
    switch (category) {
    case Category::Rule: return QStringLiteral("Rule");
    case Category::SourceClass: return QStringLiteral("Source class");
    case Category::TargetClass: return QStringLiteral("Target class");
    case Category::SourceModel: return QStringLiteral("Source model");
    case Category::TargetModel: return QStringLiteral("Target model");
    case Category::Criticity: return QStringLiteral("Criticity");
    case Category::Actor: return QStringLiteral("Actor");
    case Category::Review: return QStringLiteral("Review");
    case Category::ManualGroup: return QStringLiteral("Manual group");
    }
    return {};
}

QStringList ResultPanel::valuesOf(const ResultRow& row, Category category) {
    const auto one = [](const QString& value) { return QStringList{value.isEmpty() ? kNone : value}; };
    switch (category) {
    case Category::Rule: return one(row.rule_label);
    case Category::SourceClass: return one(row.source_class);
    case Category::TargetClass: return one(row.target_class);
    case Category::SourceModel: return one(row.source_model_name);
    case Category::TargetModel: return one(row.target_model_name);
    case Category::Criticity: return row.criticity.isEmpty() ? QStringList{kNone} : row.criticity;
    case Category::Actor: return row.actor.isEmpty() ? QStringList{kNone} : row.actor;
    case Category::Review:
        return {row.review == ReviewStatus::Valid     ? QStringLiteral("Valid")
                : row.review == ReviewStatus::Invalid ? QStringLiteral("Invalid")
                                                      : QStringLiteral("Non Determined")};
    case Category::ManualGroup: return one(row.manual_group);
    }
    return {};
}

void ResultPanel::setHiddenValues(HiddenValues hidden) {
    hidden_ = std::move(hidden);
    for (auto it = hidden_.begin(); it != hidden_.end();) it = it->second.empty() ? hidden_.erase(it) : std::next(it);
    rebuild();
}

QStringList ResultPanel::manualGroups() const {
    QStringList names;
    for (const ResultRow& row : rows_) {
        if (!row.manual_group.isEmpty() && !names.contains(row.manual_group)) names << row.manual_group;
    }
    return names;
}

void ResultPanel::setManualGroup(const std::vector<int>& rows, const QString& name) {
    for (int row : rows) {
        if (row >= 0 && row < static_cast<int>(rows_.size())) rows_[static_cast<size_t>(row)].manual_group = name;
    }
    rebuild();
    emit reviewChanged();
}

void ResultPanel::renameManualGroup(const QString& from, const QString& to) {
    for (ResultRow& row : rows_) {
        if (row.manual_group == from) row.manual_group = to;
    }
    rebuild();
    emit reviewChanged();
}

void ResultPanel::setGroupMode(GroupMode mode) {
    mode_ = mode;
    rebuild();
}

void ResultPanel::setFilterOnlyNonDetermined(bool on) {
    only_non_determined_ = on;
    rebuild();
}

void ResultPanel::setHideExcepted(bool on) {
    hide_excepted_ = on;
    rebuild();
}

void ResultPanel::clearFilters() {
    only_non_determined_ = false;
    hide_excepted_ = false;
    hidden_.clear();
    scope_.reset();
    scope_label_->setVisible(false);
    mode_ = GroupMode::Flat;
    filter_edit_->clear();
    rebuild();
}

std::vector<int> ResultPanel::shownRows() const {
    std::vector<int> shown;
    for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
        const ResultRow& row = rows_[static_cast<size_t>(i)];
        if (only_non_determined_ && row.review != ReviewStatus::NonDetermined) continue;
        if (hide_excepted_ && !row.status) continue;
        if (scope_ && scope_->count(row.node_id) == 0) continue;
        bool hidden = false;
        for (const auto& [category, values] : hidden_) {
            for (const QString& value : valuesOf(row, category)) hidden = hidden || values.count(value) > 0;
        }
        if (hidden) continue;
        if (!filter_.isEmpty()) {
            const QString haystack = row.source_name + ' ' + row.target_name + ' ' + row.rule_label + ' ' + row.source_class +
                                     ' ' + row.target_class + ' ' + row.criticity.join(' ') + ' ' + row.actor.join(' ');
            if (!haystack.contains(filter_, Qt::CaseInsensitive)) continue;
        }
        shown.push_back(i);
    }
    return shown;
}

QTreeWidgetItem* ResultPanel::appendRow(QTreeWidgetItem* parent, int index) {
    const ResultRow& row = rows_[static_cast<size_t>(index)];
    auto* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
    item->setText(kSource, row.source_name);
    item->setText(kTarget, row.target_name);
    item->setData(0, kRowRole, index);
    // The details as a tooltip; the review as the style of the row.
    QStringList tip{QString("<b>%1</b>").arg(row.rule_label.toHtmlEscaped())};
    if (!row.measure.isEmpty()) tip << row.measure.toHtmlEscaped();
    tip << "Review: " + reviewText(row.review);
    if (!row.criticity.isEmpty()) tip << "Criticity: " + row.criticity.join(", ").toHtmlEscaped();
    if (!row.actor.isEmpty()) tip << "Actor: " + row.actor.join(", ").toHtmlEscaped();
    if (!row.manual_group.isEmpty()) tip << "Manual group: " + row.manual_group.toHtmlEscaped();
    for (int c = 0; c < kColumns; ++c) item->setToolTip(c, tip.join("<br>"));
    styleRow(item, row);
    return item;
}

void ResultPanel::styleRow(QTreeWidgetItem* item, const ResultRow& row) const {
    QColor colour;  // default
    QFont font = tree_->font();
    if (row.complex && !row.status) colour = QColor("#E6194B"); // a failing Must group
    else if (!row.status || row.review == ReviewStatus::Invalid) colour = QColor(Qt::gray); // excepted / rejected
    if (row.review == ReviewStatus::Invalid) font.setStrikeOut(true);
    if (row.review == ReviewStatus::Valid) font.setBold(true);
    for (int c = 0; c < kColumns; ++c) {
        item->setForeground(c, colour.isValid() ? QBrush(colour) : QBrush());
        item->setFont(c, font);
    }
}

void ResultPanel::rebuild() {
    const QSignalBlocker blocker(tree_);
    tree_->clear();
    const std::vector<int> shown = shownRows();
    if (mode_ == GroupMode::Flat) {
        for (int i : shown) appendRow(nullptr, i);
        return;
    }
    std::vector<QString> order;
    std::map<QString, std::vector<int>> groups;
    for (int i : shown) {
        const ResultRow& row = rows_[static_cast<size_t>(i)];
        QStringList keys;
        switch (mode_) {
        case GroupMode::BySource: keys << row.source_name; break;
        case GroupMode::ByTarget: keys << row.target_name; break;
        case GroupMode::BySourceModel: keys << row.source_model_name; break;
        case GroupMode::ByRule: keys << row.rule_label; break;
        case GroupMode::ByCriticity: keys = row.criticity.isEmpty() ? QStringList{"(no criticity)"} : row.criticity; break;
        case GroupMode::ByRuleGroup: keys << (row.group.isEmpty() ? QStringLiteral("(no group)") : row.group); break;
        case GroupMode::ByManualGroup: keys << (row.manual_group.isEmpty() ? QStringLiteral("(no group)") : row.manual_group); break;
        case GroupMode::Flat: break;
        }
        for (const QString& key : keys) {
            if (key.isEmpty()) continue;
            if (!groups.count(key)) order.push_back(key);
            groups[key].push_back(i);
        }
    }
    for (const QString& key : order) {
        auto* group = new QTreeWidgetItem(tree_);
        group->setText(kSource, QString("%1 (%2)").arg(key).arg(groups[key].size()));
        group->setData(0, kGroupRole, 1);
        group->setData(0, kGroupNameRole, key);
        QFont bold = group->font(kSource);
        bold.setBold(true);
        group->setFont(kSource, bold);
        group->setFirstColumnSpanned(true);
        for (int i : groups[key]) appendRow(group, i);
        group->setExpanded(true);
    }
}

std::vector<int> ResultPanel::selectedRows() const {
    std::vector<int> rows;
    for (QTreeWidgetItem* item : tree_->selectedItems()) {
        if (item->data(0, kGroupRole).toInt() == 1) {
            for (int c = 0; c < item->childCount(); ++c) rows.push_back(rowOf(item->child(c)));
        } else if (rowOf(item) >= 0) {
            rows.push_back(rowOf(item));
        }
    }
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    return rows;
}

void ResultPanel::selectRow(int row) {
    for (QTreeWidgetItemIterator it(tree_); *it; ++it) {
        if (rowOf(*it) != row) continue;
        tree_->clearSelection();
        tree_->setCurrentItem(*it);
        (*it)->setSelected(true);
        tree_->scrollToItem(*it);
        return;
    }
}

void ResultPanel::setReview(const std::vector<int>& rows, ReviewStatus status) {
    for (int row : rows) {
        if (row >= 0 && row < static_cast<int>(rows_.size())) rows_[static_cast<size_t>(row)].review = status;
    }
    const QSignalBlocker blocker(tree_);
    for (QTreeWidgetItemIterator it(tree_); *it; ++it) {
        const int row = rowOf(*it);
        if (row >= 0) styleRow(*it, rows_[static_cast<size_t>(row)]);
    }
    emit reviewChanged();
}

void ResultPanel::contextMenu(const QPoint& pos) {
    const std::vector<int> rows = selectedRows();
    if (rows.empty()) return;
    QMenu menu(this);
    menu.setToolTipsVisible(true);
    // A group node of "Group by Manual group": rename / ungroup it.
    QTreeWidgetItem* clicked = tree_->itemAt(pos);
    if (clicked && clicked->data(0, kGroupRole).toInt() == 1 && mode_ == GroupMode::ByManualGroup &&
        clicked->data(0, kGroupNameRole).toString() != QLatin1String("(no group)")) {
        const QString name = clicked->data(0, kGroupNameRole).toString();
        menu.addAction("Rename the group…", this, [this, name]() {
            bool ok = false;
            const QString to = QInputDialog::getText(this, "Rename the group", "Name", QLineEdit::Normal, name, &ok).trimmed();
            if (ok && !to.isEmpty() && to != name) renameManualGroup(name, to);
        });
        menu.addAction("Ungroup", this, [this, name]() { renameManualGroup(name, QString()); });
        menu.addSeparator();
    }
    menu.addAction("Isolate", this, [this, rows]() { emit isolateRequested(rows); });
    menu.addAction("Show the objects", this, [this, rows]() { emit showRequested(rows); });
    menu.addSeparator();
    menu.addAction("Valid", this, [this, rows]() { setReview(rows, ReviewStatus::Valid); });
    menu.addAction("Non Determined", this, [this, rows]() { setReview(rows, ReviewStatus::NonDetermined); });
    menu.addAction("Invalid", this, [this, rows]() { setReview(rows, ReviewStatus::Invalid); });
    menu.addSeparator();
    // The user's own groups (triage): one group per result.
    QMenu* add = menu.addMenu("Add to group");
    add->setToolTipsVisible(true);
    add->addAction("New group…", this, [this, rows]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, "New group", "Name of the group", QLineEdit::Normal,
                                                   QString("Group %1").arg(manualGroups().size() + 1), &ok).trimmed();
        if (ok && !name.isEmpty()) setManualGroup(rows, name);
    });
    const QStringList groups = manualGroups();
    if (!groups.isEmpty()) add->addSeparator();
    for (const QString& name : groups) add->addAction(name, this, [this, rows, name]() { setManualGroup(rows, name); });
    bool grouped = false;
    for (int row : rows) grouped = grouped || !rows_[static_cast<size_t>(row)].manual_group.isEmpty();
    QAction* remove = menu.addAction("Remove from its group", this, [this, rows]() { setManualGroup(rows, QString()); });
    remove->setEnabled(grouped);
    // Hide the results sharing a value of the clicked one (Categories shows them again).
    const int first = clicked && rowOf(clicked) >= 0 ? rowOf(clicked) : rows.front();
    QMenu* hide = menu.addMenu("Hide the results with the same");
    for (Category category : categories()) {
        for (const QString& value : valuesOf(rows_[static_cast<size_t>(first)], category)) {
            hide->addAction(QString("%1: %2").arg(categoryName(category), value), this, [this, category, value]() {
                HiddenValues hidden = hidden_;
                hidden[category].insert(value);
                setHiddenValues(std::move(hidden));
            });
        }
    }
    menu.exec(tree_->viewport()->mapToGlobal(pos));
}

// --- CategoryFilterDialog ----------------------------------------------------------------

CategoryFilterDialog::CategoryFilterDialog(const ResultPanel& panel, QWidget* parent) : QDialog(parent) {
    setWindowTitle("Show / hide the results by category");
    resize(420, 560);
    auto* layout = new QVBoxLayout(this);
    auto* help = new QLabel("Uncheck a value to hide the results that have it; several categories add up.", this);
    help->setWordWrap(true);
    layout->addWidget(help);
    tree_ = new QTreeWidget(this);
    tree_->setHeaderLabels({"Category / value", "Results"});
    tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (ResultPanel::Category category : ResultPanel::categories()) {
        std::map<QString, int> counts;
        for (const ResultRow& row : panel.rows()) {
            for (const QString& value : ResultPanel::valuesOf(row, category)) ++counts[value];
        }
        auto* parent_item = new QTreeWidgetItem(tree_);
        parent_item->setText(0, ResultPanel::categoryName(category));
        parent_item->setData(0, Qt::UserRole, static_cast<int>(category));
        parent_item->setFlags(parent_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
        const auto found = panel.hiddenValues().find(category);
        for (const auto& [value, count] : counts) {
            auto* item = new QTreeWidgetItem(parent_item);
            item->setText(0, value);
            item->setText(1, QString::number(count));
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            const bool hidden = found != panel.hiddenValues().end() && found->second.count(value) > 0;
            item->setCheckState(0, hidden ? Qt::Unchecked : Qt::Checked);
        }
        // Categories with one value only are of little use: folded.
        parent_item->setExpanded(counts.size() > 1);
    }
    layout->addWidget(tree_, 1);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* all = buttons->addButton("Show all", QDialogButtonBox::ResetRole);
    connect(all, &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < tree_->topLevelItemCount(); ++i) tree_->topLevelItem(i)->setCheckState(0, Qt::Checked);
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

ResultPanel::HiddenValues CategoryFilterDialog::hidden() const {
    ResultPanel::HiddenValues hidden;
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem* parent_item = tree_->topLevelItem(i);
        const auto category = static_cast<ResultPanel::Category>(parent_item->data(0, Qt::UserRole).toInt());
        for (int c = 0; c < parent_item->childCount(); ++c) {
            if (parent_item->child(c)->checkState(0) == Qt::Unchecked) hidden[category].insert(parent_item->child(c)->text(0));
        }
    }
    return hidden;
}

// --- ResultInfoPanel ---------------------------------------------------------------------------

ResultInfoPanel::ResultInfoPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    title_ = new QLabel(this);
    title_->setWordWrap(true);
    layout->addWidget(title_);
    // Source / target: the element, and a button selecting it (its properties).
    const auto elementRow = [this, layout](const QString& role, QLabel** label, QToolButton** button) {
        auto* row = new QWidget(this);
        auto* row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 0, 0, 0);
        *label = new QLabel(row);
        (*label)->setWordWrap(true);
        (*label)->setTextInteractionFlags(Qt::TextSelectableByMouse);
        *button = new QToolButton(row);
        (*button)->setText("Select");
        (*button)->setIcon(components::icons::makeSvgIcon(":/icons/cursor-pointer.svg"));
        (*button)->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        (*button)->setToolTip(QString("<b>Select the %1</b><br>Selects it in the view: the Properties panel shows it.").arg(role));
        row_layout->addWidget(*label, 1);
        row_layout->addWidget(*button, 0, Qt::AlignTop);
        layout->addWidget(row);
        return row;
    };
    elementRow("source", &source_, &select_source_);
    target_row_ = elementRow("target", &target_, &select_target_);
    connect(select_source_, &QToolButton::clicked, this, [this]() { emit selectRequested(source_objects_); });
    connect(select_target_, &QToolButton::clicked, this, [this]() { emit selectRequested(target_objects_); });
    // The review.
    review_row_ = new QWidget(this);
    auto* review_layout = new QHBoxLayout(review_row_);
    review_layout->setContentsMargins(0, 0, 0, 0);
    review_layout->addWidget(new QLabel("<b>Review</b>", review_row_));
    review_ = new QComboBox(review_row_);
    review_->addItems({"Valid", "Non Determined", "Invalid"});
    review_->setItemData(0, "The clash is confirmed.", Qt::ToolTipRole);
    review_->setItemData(1, "Not reviewed yet.", Qt::ToolTipRole);
    review_->setItemData(2, "A false positive.", Qt::ToolTipRole);
    review_->setToolTip("<b>Review</b><br>The review of the result (also by the right click on the Result table).");
    connect(review_, &QComboBox::activated, this, [this](int index) { emit reviewRequested(reviewAt(index)); });
    review_layout->addWidget(review_);
    review_layout->addStretch(1);
    layout->addWidget(review_row_);
    text_ = new QLabel(this);
    text_->setWordWrap(true);
    text_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    text_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(text_, 1);
    showRow(nullptr);
}

void ResultInfoPanel::showRow(const ResultRow* row) {
    const bool shown = row != nullptr;
    for (QWidget* w : std::initializer_list<QWidget*>{source_->parentWidget(), target_row_, review_row_}) w->setVisible(shown);
    if (!shown) {
        title_->setText("<i>Select a result.</i>");
        text_->clear();
        source_objects_.clear();
        target_objects_.clear();
        return;
    }
    title_->setText(QString("<b>%1</b> <span style='color:gray'>(%2)</span>").arg(row->rule_label.toHtmlEscaped(), row->rule_type));
    const auto element = [](const QString& role, const QString& name, const QString& cls, const QString& guid, const QString& model,
                            size_t count) {
        if (count > 1) return QString("<b>%1</b>: %2 elements").arg(role).arg(count);
        return QString("<b>%1</b>: %2<br>&nbsp;&nbsp;%3 · %4 · %5").arg(role, name.toHtmlEscaped(), cls, guid, model.toHtmlEscaped());
    };
    source_->setText(element("Source", row->source_name, row->source_class, row->source_guid, row->source_model_name, row->source_objects.size()));
    source_objects_ = row->source_objects;
    select_source_->setEnabled(!source_objects_.empty());
    const bool has_target = !row->target_name.isEmpty() || row->target_objects.size() > 1;
    target_row_->setVisible(has_target);
    target_->setText(has_target ? element("Target", row->target_name, row->target_class, row->target_guid, row->target_model_name,
                                          row->target_objects.size())
                                : QString());
    target_objects_ = row->target_objects;
    select_target_->setEnabled(!target_objects_.empty());
    {
        const QSignalBlocker block(review_);
        review_->setCurrentIndex(reviewIndex(row->review));
    }
    QStringList lines;
    if (row->complex) lines << QString("<b>Must</b>: the group %1").arg(row->status ? "passes" : "<span style='color:#E6194B'>fails</span>");
    else if (!row->status) lines << "<b>Status</b>: <span style='color:gray'>excepted</span>";
    if (!row->manual_group.isEmpty()) lines << QString("<b>Manual group</b>: %1").arg(row->manual_group.toHtmlEscaped());
    if (!row->group.isEmpty()) lines << QString("<b>Group</b>: %1").arg(row->group.toHtmlEscaped());
    if (!row->criticity.isEmpty()) lines << QString("<b>Criticity</b>: %1").arg(row->criticity.join(", ").toHtmlEscaped());
    if (!row->actor.isEmpty()) lines << QString("<b>Actor</b>: %1").arg(row->actor.join(", ").toHtmlEscaped());
    if (!row->measure.isEmpty() && !row->record) lines << QString("<b>Measure</b>: %1").arg(row->measure.toHtmlEscaped());
    if (const auto& r = row->record) {
        QStringList measures;
        for (const auto& [measure, value] : r->measures.items()) {
            const QString unit = measureUnit(measure);
            measures << QString("%1 = %2%3").arg(QString::fromLatin1(ifcclash::measure_name(measure))).arg(value, 0, 'g', 6)
                            .arg(unit.isEmpty() ? QString() : " " + unit);
        }
        if (!measures.isEmpty()) lines << "<b>Measures</b>: " + measures.join(" · ");
        if (r->volumes.any()) {
            QStringList volumes;
            if (r->volumes.intersection) volumes << QString("intersection %1 m³").arg(*r->volumes.intersection, 0, 'g', 4);
            if (r->volumes.ratio) volumes << QString("ratio %1").arg(*r->volumes.ratio, 0, 'g', 4);
            if (r->volumes.containment) volumes << QString("containment %1").arg(*r->volumes.containment, 0, 'g', 4);
            if (r->volumes.clearance_violated) volumes << QString("clearance violated %1 m³").arg(*r->volumes.clearance_violated, 0, 'g', 4);
            lines << "<b>Volumes</b>: " + volumes.join(" · ");
        }
        for (size_t i = 0; i < r->points.size() && i < 4; ++i) {
            const auto& p = r->points[i];
            lines << QString("<b>Points %1</b>%2: %3 → %4").arg(i + 1)
                         .arg(p.label ? " (" + QString::fromStdString(*p.label) + ")" : QString())
                         .arg(formatPoint(p.source), formatPoint(p.target));
        }
        QStringList blocks;
        if (!r->faces.empty()) blocks << QString("%1 face pair(s)").arg(r->faces.size());
        if (!r->source_obbs.empty() || !r->target_obbs.empty()) blocks << QString("%1 box(es)").arg(r->source_obbs.size() + r->target_obbs.size());
        if (!r->axes.empty()) blocks << QString("%1 axis(es)").arg(r->axes.size());
        if (r->raycast) blocks << QString("rays: %1 hit of %2 cast").arg(r->raycast->hit).arg(r->raycast->cast);
        if (r->axis && r->axis->mode) blocks << QString("axis mode %1%2").arg(QString::fromStdString(*r->axis->mode), r->axis->degraded.value_or(false) ? " (degraded)" : "");
        if (!blocks.isEmpty()) lines << "<b>Geometry</b>: " + blocks.join(" · ");
        if (r->flags.any()) lines << "<span style='color:gray'>Degraded geometry (fallback or gross element).</span>";
    }
    text_->setText(lines.join("<br>"));
}

} // namespace bonsaiviewer::modules::rule
