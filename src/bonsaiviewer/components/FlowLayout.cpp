// This file was generated with the assistance of an AI coding tool.
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

#include "FlowLayout.h"

#include <QResizeEvent>
#include <QWidget>

#include <algorithm>

namespace bonsaiviewer::components {

FlowLayout::FlowLayout(QWidget* parent, int spacing) : QLayout(parent), spacing_(spacing) {
    setContentsMargins(0, 0, 0, 0);
}

FlowLayout::~FlowLayout() {
    while (QLayoutItem* item = takeAt(0)) delete item;
}

void FlowLayout::setWrapping(bool wrapping) {
    if (wrapping_ == wrapping) return;
    wrapping_ = wrapping;
    invalidate();
}

void FlowLayout::addItem(QLayoutItem* item) {
    items_.append(item);
}

int FlowLayout::count() const {
    return static_cast<int>(items_.size());
}

QLayoutItem* FlowLayout::itemAt(int index) const {
    return index >= 0 && index < items_.size() ? items_.at(index) : nullptr;
}

QLayoutItem* FlowLayout::takeAt(int index) {
    return index >= 0 && index < items_.size() ? items_.takeAt(index) : nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const {
    return {};
}

bool FlowLayout::hasHeightForWidth() const {
    return true;
}

int FlowLayout::heightForWidth(int width) const {
    return doLayout(QRect(0, 0, width, 0), true);
}

QSize FlowLayout::minimumSize() const {
    // The widest item: below, nothing fits (one row: the end of the row is cut,
    // the window is not forced wider).
    QSize size;
    for (const QLayoutItem* item : items_) size = size.expandedTo(item->minimumSize());
    const QMargins m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
}

QSize FlowLayout::sizeHint() const {
    // One row: every item side by side.
    int width = 0, height = 0;
    for (const QLayoutItem* item : items_) {
        if (item->isEmpty()) continue;
        if (width > 0) width += spacing_;
        width += item->sizeHint().width();
        height = std::max(height, item->sizeHint().height());
    }
    const QMargins m = contentsMargins();
    return {width + m.left() + m.right(), height + m.top() + m.bottom()};
}

void FlowLayout::setGeometry(const QRect& rect) {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

int FlowLayout::doLayout(const QRect& rect, bool test_only) const {
    const QMargins m = contentsMargins();
    const QRect area = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
    int x = area.x(), y = area.y(), line_height = 0;
    for (QLayoutItem* item : items_) {
        if (item->isEmpty()) continue;
        // An item that takes its height from its width (a nested flow) gets
        // the whole row when it cannot fit in one line.
        QSize size = item->sizeHint();
        if (wrapping_ && size.width() > area.width() && item->hasHeightForWidth()) {
            size = QSize(std::max(area.width(), item->minimumSize().width()), item->heightForWidth(area.width()));
        }
        int next = x + size.width();
        if (wrapping_ && next > area.right() + 1 && line_height > 0) {
            x = area.x();
            y += line_height + spacing_;
            next = x + size.width();
            line_height = 0;
        }
        if (!test_only) item->setGeometry(QRect(QPoint(x, y), size));
        x = next + spacing_;
        line_height = std::max(line_height, size.height());
    }
    return y + line_height - rect.y() + m.bottom();
}

void FlowFrame::setWrapping(bool wrapping) {
    for (QLayout* layout : findChildren<QLayout*>()) {
        if (auto* flow = dynamic_cast<FlowLayout*>(layout)) flow->setWrapping(wrapping);
    }
    if (auto* flow = dynamic_cast<FlowLayout*>(layout())) flow->setWrapping(wrapping);
    setMinimumHeight(layout() ? layout()->heightForWidth(width()) : 0);
    // The containers above cache the old sizes (a dock panel, its scroll area):
    // they are recomputed up to the window.
    for (QWidget* w = this; w != nullptr; w = w->parentWidget()) {
        if (w->layout()) w->layout()->invalidate();
        w->updateGeometry();
        if (w->isWindow()) break;
    }
}

void FlowFrame::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    if (layout() == nullptr || !layout()->hasHeightForWidth()) return;
    const int height = layout()->heightForWidth(event->size().width());
    if (height != minimumHeight()) setMinimumHeight(height);
}

} // namespace bonsaiviewer::components
