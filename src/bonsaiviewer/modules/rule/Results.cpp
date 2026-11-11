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

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include <algorithm>
#include <map>

namespace bonsaiviewer::modules::rule {

namespace {

constexpr int kRowRole = Qt::UserRole + 10;
constexpr int kGroupRole = Qt::UserRole + 11;

enum Column { kRule, kSource, kTarget, kModel, kMeasure, kCriticity, kActor, kValid, kUndetermined, kInvalid, kColumns };

int rowOf(const QTreeWidgetItem* item) {
    const QVariant v = item->data(0, kRowRole);
    return v.isValid() ? v.toInt() : -1;
}

void setChecks(QTreeWidgetItem* item, ReviewStatus status) {
    item->setCheckState(kValid, status == ReviewStatus::Valid ? Qt::Checked : Qt::Unchecked);
    item->setCheckState(kUndetermined, status == ReviewStatus::NonDetermined ? Qt::Checked : Qt::Unchecked);
    item->setCheckState(kInvalid, status == ReviewStatus::Invalid ? Qt::Checked : Qt::Unchecked);
}

QString modelText(const ResultRow& row) {
    if (row.target_model_name.isEmpty() || row.source_model_name == row.target_model_name) return row.source_model_name;
    return QString("%1 → %2").arg(row.source_model_name, row.target_model_name);
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
    tree_ = new QTreeWidget(this);
    tree_->setColumnCount(kColumns);
    tree_->setHeaderLabels({"Rule", "Source", "Target", "Model", "Measure", "Criticity", "Actor", "Valid",
                            "Non Determined", "Invalid"});
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->setRootIsDecorated(true);
    tree_->header()->setSectionResizeMode(kSource, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(kTarget, QHeaderView::Stretch);
    tree_->headerItem()->setToolTip(kValid, "The clash is confirmed.");
    tree_->headerItem()->setToolTip(kUndetermined, "Not reviewed yet.");
    tree_->headerItem()->setToolTip(kInvalid, "A false positive.");
    layout->addWidget(tree_, 1);
    connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int column) {
        const int row = rowOf(item);
        if (row >= 0 && column >= kValid && column <= kInvalid) {
            setReview({row}, column == kValid ? ReviewStatus::Valid
                             : column == kInvalid ? ReviewStatus::Invalid
                                                  : ReviewStatus::NonDetermined);
        }
    });
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
    item->setText(kRule, row.rule_label);
    item->setText(kSource, row.source_name);
    item->setText(kTarget, row.target_name);
    item->setText(kModel, modelText(row));
    item->setText(kMeasure, row.measure);
    item->setText(kCriticity, row.criticity.join(", "));
    item->setText(kActor, row.actor.join(", "));
    item->setToolTip(kSource, QString("%1\n%2").arg(row.source_class, row.source_guid));
    item->setToolTip(kTarget, QString("%1\n%2").arg(row.target_class, row.target_guid));
    item->setData(0, kRowRole, index);
    setChecks(item, row.review);
    if (!row.status) {
        // Excepted (or a failing Must group): shown apart.
        for (int c = 0; c < kValid; ++c) item->setForeground(c, QBrush(QColor(Qt::gray)));
        item->setToolTip(kRule, row.complex ? "This group fails the Must check." : "Excepted: this pair matches an exception of the rule.");
        if (row.complex) {
            for (int c = 0; c < kValid; ++c) item->setForeground(c, QBrush(QColor("#E6194B")));
        }
    }
    return item;
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
        case GroupMode::ByRule: keys << row.rule_label; break;
        case GroupMode::ByCriticity: keys = row.criticity.isEmpty() ? QStringList{"(no criticity)"} : row.criticity; break;
        case GroupMode::ByRuleGroup: keys << (row.group.isEmpty() ? QStringLiteral("(no group)") : row.group); break;
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
        group->setText(kRule, key);
        group->setText(kModel, QString("(%1)").arg(groups[key].size()));
        group->setData(0, kGroupRole, 1);
        QFont bold = group->font(kRule);
        bold.setBold(true);
        group->setFont(kRule, bold);
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

void ResultPanel::setReview(const std::vector<int>& rows, ReviewStatus status) {
    for (int row : rows) {
        if (row >= 0 && row < static_cast<int>(rows_.size())) rows_[static_cast<size_t>(row)].review = status;
    }
    const QSignalBlocker blocker(tree_);
    for (QTreeWidgetItemIterator it(tree_); *it; ++it) {
        const int row = rowOf(*it);
        if (row >= 0) setChecks(*it, rows_[static_cast<size_t>(row)].review);
    }
    emit reviewChanged();
}

void ResultPanel::contextMenu(const QPoint& pos) {
    const std::vector<int> rows = selectedRows();
    if (rows.empty()) return;
    QMenu menu(this);
    menu.addAction("Isolate", this, [this, rows]() { emit isolateRequested(rows); });
    menu.addAction("Show the objects", this, [this, rows]() { emit showRequested(rows); });
    menu.addSeparator();
    menu.addAction("Valid", this, [this, rows]() { setReview(rows, ReviewStatus::Valid); });
    menu.addAction("Non Determined", this, [this, rows]() { setReview(rows, ReviewStatus::NonDetermined); });
    menu.addAction("Invalid", this, [this, rows]() { setReview(rows, ReviewStatus::Invalid); });
    menu.exec(tree_->viewport()->mapToGlobal(pos));
}

// --- ResultInfoPanel ---------------------------------------------------------------------------

ResultInfoPanel::ResultInfoPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    text_ = new QLabel(this);
    text_->setWordWrap(true);
    text_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    text_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(text_, 1);
    showRow(nullptr);
}

void ResultInfoPanel::showRow(const ResultRow* row) {
    if (row == nullptr) {
        text_->setText("<i>Select a result.</i>");
        return;
    }
    QStringList lines;
    lines << QString("<b>%1</b> <span style='color:gray'>(%2)</span>").arg(row->rule_label.toHtmlEscaped(), row->rule_type);
    const auto element = [](const QString& role, const QString& name, const QString& cls, const QString& guid, const QString& model,
                            size_t count) {
        if (count > 1) return QString("<b>%1</b>: %2 elements").arg(role).arg(count);
        return QString("<b>%1</b>: %2<br>&nbsp;&nbsp;%3 · %4 · %5").arg(role, name.toHtmlEscaped(), cls, guid, model.toHtmlEscaped());
    };
    lines << element("Source", row->source_name, row->source_class, row->source_guid, row->source_model_name, row->source_objects.size());
    if (!row->target_name.isEmpty() || row->target_objects.size() > 1) {
        lines << element("Target", row->target_name, row->target_class, row->target_guid, row->target_model_name, row->target_objects.size());
    }
    if (row->complex) lines << QString("<b>Must</b>: the group %1").arg(row->status ? "passes" : "<span style='color:#E6194B'>fails</span>");
    else if (!row->status) lines << "<b>Status</b>: <span style='color:gray'>excepted</span>";
    if (!row->group.isEmpty()) lines << QString("<b>Group</b>: %1").arg(row->group.toHtmlEscaped());
    if (!row->criticity.isEmpty()) lines << QString("<b>Criticity</b>: %1").arg(row->criticity.join(", ").toHtmlEscaped());
    if (!row->actor.isEmpty()) lines << QString("<b>Actor</b>: %1").arg(row->actor.join(", ").toHtmlEscaped());
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
