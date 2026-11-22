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

#include "GridPanel.h"

#include "../../components/Section.h"
#include "../../components/SvgIcon.h"

#include <QAbstractTableModel>
#include <QApplication>
#include <QBrush>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSortFilterProxyModel>
#include <QStyleOptionHeader>
#include <QTableView>
#include <QToolButton>
#include <QWidgetAction>

#include <algorithm>
#include <cmath>
#include <map>

namespace bonsaiviewer::modules::external_documents {

namespace {

// The model columns prepended to the file's columns.
constexpr int kModelColumnCount = 3; // Class, Name, GlobalId

// Header icons, same geometry as the Table panel header (TableHeader.cpp).
constexpr int kIconSize = 12;
constexpr int kIconGap = 4;
// Room taken on the right of each section by the arrow and the funnel.
constexpr int kIconsWidth = 2 * kIconSize + 3 * kIconGap;

// Alphabetical, locale-aware, case-insensitive; ties broken case-sensitively.
bool textLess(const QString& a, const QString& b) {
    const int order = QString::localeAwareCompare(a.toLower(), b.toLower());
    return order != 0 ? order < 0 : a < b;
}

// Same number rule as the Table panel (TableModel::autoNumeric): a comma is
// the decimal separator ("3,14"), unless a dot is there too.
bool isNumber(const QString& text, double& out) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || (trimmed.contains('.') && trimmed.contains(','))) return false;
    bool ok = false;
    const double value = QString(trimmed).replace(',', '.').toDouble(&ok);
    // "nan" / "inf" parse as numbers; a NaN would break the strict weak
    // ordering the sort relies on.
    if (!ok || !std::isfinite(value)) return false;
    out = value;
    return true;
}

} // namespace

// --- model -------------------------------------------------------------------

class ExternalGridModel : public QAbstractTableModel {
    Q_OBJECT
public:
    ExternalGridModel(const TableData& data, const JoinResult& join, const ExternalGridPanel::RowInfo& row_info,
                      QObject* parent = nullptr)
        : QAbstractTableModel(parent), data_(data), join_(join), row_info_(row_info)
    {
        columns_ << QStringLiteral("Class") << QStringLiteral("Name") << QStringLiteral("GlobalId");
        columns_ << data.columns;
    }

    int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : data_.rows.size();
    }
    int columnCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : columns_.size();
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < columns_.size())
            return columns_.at(section);
        return {};
    }
    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid()) return {};
        const int row = index.row();
        const int column = index.column();
        const std::uint32_t object_id = row_info_.object_of_row ? row_info_.object_of_row(row) : 0;
        const bool matched = object_id != 0;
        if (role == Qt::ForegroundRole && !matched)
            return QBrush(QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
        if (role != Qt::DisplayRole) return {};
        if (column < kModelColumnCount) {
            if (!matched) return {};
            const QStringList info = row_info_.basic_info_of ? row_info_.basic_info_of(object_id) : QStringList();
            return info.size() >= 3 ? info.at(column) : QString();
        }
        const QStringList& values = data_.rows.at(row);
        const int file_column = column - kModelColumnCount;
        return file_column < values.size() ? values.at(file_column) : QString();
    }

    QString cellText(int row, int column) const {
        return data(index(row, column), Qt::DisplayRole).toString();
    }

private:
    TableData data_;
    JoinResult join_;
    ExternalGridPanel::RowInfo row_info_;
    QStringList columns_;
};

// --- proxy -------------------------------------------------------------------

class ExternalGridProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ExternalGridProxy(ExternalGridModel* model, QObject* parent = nullptr)
        : QSortFilterProxyModel(parent), model_(model) { setSortCaseSensitivity(Qt::CaseInsensitive); }

    QString cellText(int source_row, int column) const { return model_->cellText(source_row, column); }
    QStringList columnValues(int column) const {
        QSet<QString> values;
        for (int row = 0; row < model_->rowCount(); ++row) values.insert(cellText(row, column));
        QStringList sorted(values.begin(), values.end());
        std::sort(sorted.begin(), sorted.end(), [](const QString& a, const QString& b) {
            if (a.isEmpty() != b.isEmpty()) return b.isEmpty(); // empty last
            return textLess(a, b);
        });
        return sorted;
    }
    void setColumnFilter(int column, const QSet<QString>& kept) {
        if (kept.isEmpty()) filters_.erase(column);
        else filters_[column] = kept;
        invalidateFilter();
    }
    void clearColumnFilter(int column) { filters_.erase(column); invalidateFilter(); }
    bool isFiltered(int column) const { return filters_.count(column) > 0; }
    QSet<QString> columnFilter(int column) const {
        const auto it = filters_.find(column);
        return it != filters_.end() ? it->second : QSet<QString>();
    }
    int sourceRow(int displayed_row) const {
        const QModelIndex index = mapToSource(this->index(displayed_row, 0));
        return index.isValid() ? index.row() : -1;
    }

protected:
    // Same order as the Table panel (TableProxy::lessThan).
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
        const QString a = left.data(Qt::DisplayRole).toString();
        const QString b = right.data(Qt::DisplayRole).toString();
        // Empty cells stay at the end, whatever the order.
        const bool descending = sortOrder() == Qt::DescendingOrder;
        if (a.isEmpty() != b.isEmpty()) return descending ? a.isEmpty() : b.isEmpty();
        // Numbers before texts; numbers by value, texts alphabetically.
        double da, db;
        const bool na = isNumber(a, da), nb = isNumber(b, db);
        if (na != nb) return na;
        if (na) return da < db;
        return textLess(a, b);
    }
    bool filterAcceptsRow(int source_row, const QModelIndex&) const override {
        for (const auto& [column, kept] : filters_) {
            if (!kept.contains(model_->cellText(source_row, column))) return false;
        }
        return true;
    }

private:
    ExternalGridModel* model_;
    std::map<int, QSet<QString>> filters_;
};

// --- header (sort arrow + funnel, same icons as the Table panel header) ------

class ExternalGridHeader : public QHeaderView {
    Q_OBJECT
public:
    explicit ExternalGridHeader(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QHeaderView(orientation, parent) {
        setSectionsMovable(true); // columns can be moved by left drag on the header
        setStretchLastSection(true);
    }

    // order: 0 = A→Z, 1 = Z→A, 2 = back to the file order.
    void setSortState(int section, int order) {
        sort_section_ = section;
        sort_order_ = order;
        viewport()->update();
    }
    void setFiltered(int section, bool filtered) {
        if (filtered) filtered_.insert(section);
        else filtered_.remove(section);
        viewport()->update();
    }
    int sortSection() const { return sort_section_; }
    int sortOrder() const { return sort_order_; }

signals:
    void sortCycleRequested(int section); // the header cycles A→Z, Z→A, file order itself
    void filterRequested(int section, const QPoint& global_pos);

protected:
    QSize sectionSizeFromContents(int logical_index) const override {
        QSize size = QHeaderView::sectionSizeFromContents(logical_index);
        size.rwidth() += kIconsWidth;
        return size;
    }

    void paintSection(QPainter* painter, const QRect& rect, int logical_index) const override {
        if (!rect.isValid()) return;
        // Section frame on the whole rect, title left of the icons.
        QStyleOptionHeader option;
        initStyleOptionForIndex(&option, logical_index);
        option.rect = rect;
        option.text.clear();
        style()->drawControl(QStyle::CE_HeaderSection, &option, painter, this);
        initStyleOptionForIndex(&option, logical_index);
        option.rect = rect.adjusted(0, 0, -kIconsWidth, 0);
        option.sortIndicator = QStyleOptionHeader::None;
        style()->drawControl(QStyle::CE_HeaderLabel, &option, painter, this);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QColor idle = palette().color(QPalette::Disabled, QPalette::Text);
        const QColor active = palette().color(QPalette::Highlight);

        // Sort arrow: a triangle, down for A→Z, up for Z→A.
        const QRect arrow = sortRect(rect).adjusted(1, 2, -1, -2);
        const bool sorted = logical_index == sort_section_;
        const bool up = sorted && sort_order_ == 1;
        QPainterPath triangle;
        if (up) {
            triangle.moveTo(arrow.left(), arrow.bottom());
            triangle.lineTo(arrow.right(), arrow.bottom());
            triangle.lineTo(arrow.center().x() + 0.5, arrow.top());
        } else {
            triangle.moveTo(arrow.left(), arrow.top());
            triangle.lineTo(arrow.right(), arrow.top());
            triangle.lineTo(arrow.center().x() + 0.5, arrow.bottom());
        }
        triangle.closeSubpath();
        painter->fillPath(triangle, sorted ? active : idle);

        // Filter funnel: a trapezoid over a short stem — the Table panel's icon.
        const QRectF funnel = filterRect(rect);
        const bool filtered = filtered_.contains(logical_index);
        QPainterPath path;
        path.moveTo(funnel.left(), funnel.top() + 1);
        path.lineTo(funnel.right(), funnel.top() + 1);
        path.lineTo(funnel.center().x() + 1.5, funnel.center().y() + 1);
        path.lineTo(funnel.center().x() + 1.5, funnel.bottom());
        path.lineTo(funnel.center().x() - 1.5, funnel.bottom() - 1.5);
        path.lineTo(funnel.center().x() - 1.5, funnel.center().y() + 1);
        path.closeSubpath();
        if (filtered) {
            painter->fillPath(path, active);
        } else {
            painter->setPen(QPen(idle, 1.2));
            painter->drawPath(path);
        }
        painter->restore();
    }

    void mousePressEvent(QMouseEvent* event) override {
        // Same as the Table panel header: the arrow cycles the sort, the
        // funnel opens the filter; neither starts a column drag.
        const int section = logicalIndexAt(event->pos());
        if (section >= 0 && event->button() == Qt::LeftButton) {
            const QRect rect = sectionRectOf(section);
            // Slightly larger hit areas than the icons.
            if (sortRect(rect).adjusted(-2, -4, 2, 4).contains(event->pos())) {
                emit sortCycleRequested(section);
                event->accept();
                return;
            }
            if (filterRect(rect).adjusted(-2, -4, 2, 4).contains(event->pos())) {
                emit filterRequested(section, event->globalPosition().toPoint());
                event->accept();
                return;
            }
        }
        QHeaderView::mousePressEvent(event);
    }

private:
    QRect filterRect(const QRect& section) const {
        return {section.right() - kIconGap - kIconSize + 1, section.center().y() - kIconSize / 2, kIconSize, kIconSize};
    }
    QRect sortRect(const QRect& section) const {
        return filterRect(section).translated(-(kIconSize + kIconGap), 0);
    }
    QRect sectionRectOf(int logical_index) const {
        return QRect(sectionViewportPosition(logical_index), 0, sectionSize(logical_index), height());
    }

    int sort_section_ = -1;
    int sort_order_ = 0;
    QSet<int> filtered_;
};

// --- panel -------------------------------------------------------------------

ExternalGridPanel::ExternalGridPanel(QWidget* parent)
    : components::Panel("External Table", nullptr, parent)
{
    auto* section = new components::Section("", components::SectionHeaderMode::Hidden, this);
    section->setBodyExpanding(true);

    auto* toolbar = new QWidget(section);
    auto* toolbar_layout = new QHBoxLayout(toolbar);
    toolbar_layout->setContentsMargins(0, 0, 0, 0);
    toolbar_layout->setSpacing(2);
    title_label_ = new QLabel(toolbar);
    title_label_->setStyleSheet(QStringLiteral("font-weight: 600;"));
    refresh_button_ = new QToolButton(toolbar);
    refresh_button_->setIcon(components::icons::makeAccentSvgIcon(":/icons/refresh-double.svg"));
    refresh_button_->setToolTip("Re-read the file from disk and re-apply the join");
    refresh_button_->setAutoRaise(true);
    edit_join_button_ = new QToolButton(toolbar);
    edit_join_button_->setIcon(components::icons::makeAccentSvgIcon(":/icons/filter.svg"));
    edit_join_button_->setToolTip("Edit the join configuration (model key, key column, sheet)");
    edit_join_button_->setAutoRaise(true);
    export_button_ = new QToolButton(toolbar);
    export_button_->setIcon(components::icons::makeAccentSvgIcon(":/icons/floppy-disk.svg"));
    export_button_->setToolTip("Export all the rows of the file to CSV");
    export_button_->setAutoRaise(true);
    report_button_ = new QToolButton(toolbar);
    report_button_->setIcon(components::icons::makeAccentSvgIcon(":/icons/search.svg"));
    report_button_->setToolTip("Match report: unmatched rows, duplicate rows, unmatched objects");
    report_button_->setAutoRaise(true);
    counts_label_ = new QLabel(toolbar);
    toolbar_layout->addWidget(title_label_);
    toolbar_layout->addWidget(refresh_button_);
    toolbar_layout->addWidget(edit_join_button_);
    toolbar_layout->addWidget(export_button_);
    toolbar_layout->addWidget(report_button_);
    toolbar_layout->addWidget(counts_label_);
    toolbar_layout->addStretch(1);
    section->addBodyWidget(toolbar);

    view_ = new QTableView(section);
    view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view_->setSortingEnabled(false);
    view_->verticalHeader()->setVisible(false);
    section->addBodyWidget(view_);
    addBodyWidget(section);

    connect(refresh_button_, &QToolButton::clicked, this, &ExternalGridPanel::refreshRequested);
    connect(edit_join_button_, &QToolButton::clicked, this, &ExternalGridPanel::editJoinRequested);
    connect(export_button_, &QToolButton::clicked, this, &ExternalGridPanel::exportCsvRequested);
    connect(report_button_, &QToolButton::clicked, this, &ExternalGridPanel::showReportRequested);

    // Click on a row: select the joined object(s) of the selected rows
    // (several rows with Ctrl / Shift).
    connect(view_, &QTableView::clicked, this, [this](const QModelIndex& index) {
        if (!index.isValid() || proxy_ == nullptr) return;
        const bool add = (QApplication::keyboardModifiers() & Qt::ControlModifier) != 0;
        emit rowsSelected(selectedRowObjectIds(), add);
    });
    // Double-click: isolate the joined object(s); when the clicked row is not
    // part of the selection, only that row is isolated.
    connect(view_, &QTableView::doubleClicked, this, [this](const QModelIndex& index) {
        if (!index.isValid() || proxy_ == nullptr) return;
        auto* proxy = static_cast<ExternalGridProxy*>(proxy_);
        QList<std::uint32_t> ids;
        if (view_->selectionModel()->isSelected(index)) {
            ids = selectedRowObjectIds();
        } else {
            const int row = proxy->sourceRow(index.row());
            const std::uint32_t object_id = row_info_.object_of_row ? row_info_.object_of_row(row) : 0;
            if (object_id != 0) ids.append(object_id);
        }
        if (!ids.isEmpty()) emit rowsIsolateRequested(ids);
    });
    // Right-click on a row: Isolate, same rule as the double-click.
    view_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view_, &QTableView::customContextMenuRequested, this, [this](const QPoint& pos) {
        const QModelIndex index = view_->indexAt(pos);
        if (!index.isValid() || proxy_ == nullptr) return;
        QMenu menu(view_);
        menu.addAction(QStringLiteral("Isolate"), this, [this, index]() {
            auto* proxy = static_cast<ExternalGridProxy*>(proxy_);
            QList<std::uint32_t> ids;
            if (view_->selectionModel()->isSelected(index)) {
                ids = selectedRowObjectIds();
            } else {
                const int row = proxy->sourceRow(index.row());
                const std::uint32_t object_id = row_info_.object_of_row ? row_info_.object_of_row(row) : 0;
                if (object_id != 0) ids.append(object_id);
            }
            if (!ids.isEmpty()) emit rowsIsolateRequested(ids);
        });
        menu.exec(view_->viewport()->mapToGlobal(pos));
    });
}

void ExternalGridPanel::showTable(const ExternalDocument& document, const TableData& data,
                                  const JoinResult& join, const RowInfo& row_info) {
    document_id_ = document.id;
    data_ = data;
    join_ = join;
    row_info_ = row_info;

    // Swap model/proxy/header without ever deleting the live model: the new
    // pair is built first, the view is detached (setModel(nullptr)), the old
    // pair deleted, then the header is reinstalled and explicitly bound to
    // the new proxy — otherwise the header ends up with no model after a
    // re-join and the columns vanish.
    auto* new_model = new ExternalGridModel(data, join, row_info, this);
    auto* new_proxy = new ExternalGridProxy(new_model, this);
    new_proxy->setSourceModel(new_model);
    view_->setModel(nullptr);
    delete proxy_;
    delete model_;
    model_ = new_model;
    proxy_ = new_proxy;
    // setHorizontalHeader: the view deletes the previous header itself.
    auto* header = new ExternalGridHeader(Qt::Horizontal, view_);
    header_ = header;
    view_->setHorizontalHeader(header_);
    view_->setModel(proxy_);
    header_->setModel(proxy_);
    // A header installed while the view is already visible stays hidden
    // (setHorizontalHeader does not show it): without this, a re-join
    // (Visualize → Edit Join → Visualize) hides the column titles.
    header_->setVisible(true);

    // Sort cycle on the header arrow: A→Z, Z→A, then back to the file order.
    connect(header, &ExternalGridHeader::sortCycleRequested, this, [this, header](int section) {
        auto* proxy = static_cast<ExternalGridProxy*>(proxy_);
        int order = 0;
        if (header->sortSection() == section) {
            order = header->sortOrder() == 0 ? 1 : 2;
        }
        if (order == 2) {
            proxy->sort(-1);
            header->setSortState(-1, 0);
        } else {
            proxy->sort(section, order == 0 ? Qt::AscendingOrder : Qt::DescendingOrder);
            header->setSortState(section, order);
        }
    });
    connect(header, &ExternalGridHeader::filterRequested, this, [this](int section, const QPoint& global_pos) {
        buildFilterMenu(section, global_pos);
    });

    title_label_->setText(document.name);
    setCounts(QStringLiteral("%1 of %2 row(s), %3 matched")
                  .arg(data.rows.size()).arg(data.total_rows).arg(join.report.matched_rows));
    show();
    raise();
}

void ExternalGridPanel::clearTable() {
    document_id_.clear();
    delete model_; model_ = nullptr;
    delete proxy_; proxy_ = nullptr;
    view_->setModel(nullptr);
    view_->setHorizontalHeader(new QHeaderView(Qt::Horizontal, view_));
    header_ = view_->horizontalHeader();
    title_label_->clear();
    counts_label_->clear();
}

void ExternalGridPanel::setCounts(const QString& text) {
    counts_label_->setText(text);
}

void ExternalGridPanel::buildFilterMenu(int column, const QPoint& global_pos) {
    if (proxy_ == nullptr) return;
    auto* proxy = static_cast<ExternalGridProxy*>(proxy_);
    auto* header = static_cast<ExternalGridHeader*>(header_);
    const QStringList values = proxy->columnValues(column);
    QSet<QString> kept = proxy->columnFilter(column);
    if (!proxy->isFiltered(column)) {
        for (const QString& value : values) kept.insert(value);
    }

    QMenu menu(this);
    auto* search = new QLineEdit(&menu);
    search->setPlaceholderText(QStringLiteral("Search"));
    auto* search_action = new QWidgetAction(&menu);
    search_action->setDefaultWidget(search);
    menu.addAction(search_action);
    menu.addSeparator();
    auto* all_action = menu.addAction(QStringLiteral("All"));
    auto* none_action = menu.addAction(QStringLiteral("None"));
    menu.addSeparator();
    QList<QAction*> check_actions;
    for (const QString& value : values) {
        auto* action = menu.addAction(value.isEmpty() ? QStringLiteral("(empty)") : value);
        action->setCheckable(true);
        action->setChecked(kept.contains(value));
        check_actions.append(action);
    }
    auto* apply_action = menu.addAction(QStringLiteral("Apply"));
    connect(search, &QLineEdit::textChanged, this, [&check_actions](const QString& text) {
        for (QAction* action : check_actions) {
            action->setVisible(text.isEmpty() || action->text().contains(text, Qt::CaseInsensitive));
        }
    });
    connect(all_action, &QAction::triggered, this, [&check_actions]() {
        for (QAction* action : check_actions) action->setChecked(true);
    });
    connect(none_action, &QAction::triggered, this, [&check_actions]() {
        for (QAction* action : check_actions) action->setChecked(false);
    });
    menu.exec(global_pos);
    if (menu.activeAction() != apply_action) return;
    QSet<QString> new_kept;
    for (QAction* action : check_actions) {
        if (!action->isChecked()) continue;
        new_kept.insert(action->text() == QStringLiteral("(empty)") ? QString() : action->text());
    }
    if (new_kept.size() == values.size()) {
        proxy->clearColumnFilter(column);
        header->setFiltered(column, false);
    } else {
        proxy->setColumnFilter(column, new_kept);
        header->setFiltered(column, true);
    }
    setCounts(QStringLiteral("%1 of %2 row(s)").arg(proxy->rowCount()).arg(data_.total_rows));
}

QList<std::uint32_t> ExternalGridPanel::selectedRowObjectIds() const {
    QList<std::uint32_t> ids;
    if (proxy_ == nullptr || view_->selectionModel() == nullptr || !row_info_.object_of_row) return ids;
    const QModelIndexList rows = view_->selectionModel()->selectedRows();
    for (const QModelIndex& proxy_index : rows) {
        const int source_row = static_cast<ExternalGridProxy*>(proxy_)->sourceRow(proxy_index.row());
        if (source_row < 0) continue;
        const std::uint32_t object_id = row_info_.object_of_row(source_row);
        if (object_id != 0) ids.append(object_id);
    }
    return ids;
}

} // namespace bonsaiviewer::modules::external_documents

#include "GridPanel.moc"
