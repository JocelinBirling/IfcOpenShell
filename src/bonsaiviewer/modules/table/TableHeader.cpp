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

#include "TableHeader.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionHeader>

namespace bonsaiviewer::modules::table {

namespace {

constexpr int kIconSize = 12;
constexpr int kIconGap = 4;
// Room taken on the right of each section by the arrow and the funnel.
constexpr int kIconsWidth = 2 * kIconSize + 3 * kIconGap;

} // namespace

TableHeader::TableHeader(QWidget* parent)
    : QHeaderView(Qt::Horizontal, parent) {
    setSectionsMovable(true);
    setSectionsClickable(true);
    setHighlightSections(false);
}

void TableHeader::setSortState(int section, Qt::SortOrder order) {
    sort_section_ = section;
    sort_order_ = order;
    viewport()->update();
}

void TableHeader::setFiltered(int section, bool filtered) {
    if (filtered) filtered_.insert(section);
    else filtered_.remove(section);
    viewport()->update();
}

void TableHeader::clearStates() {
    sort_section_ = -1;
    filtered_.clear();
    viewport()->update();
}

QRect TableHeader::filterRect(const QRect& section) const {
    return {section.right() - kIconGap - kIconSize + 1, section.center().y() - kIconSize / 2, kIconSize, kIconSize};
}

QRect TableHeader::sortRect(const QRect& section) const {
    return filterRect(section).translated(-(kIconSize + kIconGap), 0);
}

QRect TableHeader::sectionRect(int logical_index) const {
    return {sectionViewportPosition(logical_index), 0, sectionSize(logical_index), height()};
}

QSize TableHeader::sectionSizeFromContents(int logical_index) const {
    QSize size = QHeaderView::sectionSizeFromContents(logical_index);
    size.rwidth() += kIconsWidth;
    return size;
}

void TableHeader::paintSection(QPainter* painter, const QRect& rect, int logical_index) const {
    if (!rect.isValid()) return;
    // The section frame on the whole rect, the title left of the icons.
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

    // Sort arrow: a triangle, down for A to Z, up for Z to A.
    const QRect arrow = sortRect(rect).adjusted(1, 2, -1, -2);
    const bool sorted = logical_index == sort_section_;
    const bool up = sorted && sort_order_ == Qt::DescendingOrder;
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

    // Funnel: a trapezoid over a short stem.
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

void TableHeader::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        const QPoint pos = event->position().toPoint();
        const int section = logicalIndexAt(pos);
        if (section >= 0) {
            const QRect rect = sectionRect(section);
            // Slightly larger hit areas than the icons.
            if (sortRect(rect).adjusted(-2, -4, 2, 4).contains(pos)) {
                emit sortRequested(section);
                event->accept();
                return;
            }
            if (filterRect(rect).adjusted(-2, -4, 2, 4).contains(pos)) {
                emit filterRequested(section, mapToGlobal(QPoint(rect.right() - kIconsWidth, height())));
                event->accept();
                return;
            }
        }
    }
    QHeaderView::mousePressEvent(event);
}

} // namespace bonsaiviewer::modules::table
