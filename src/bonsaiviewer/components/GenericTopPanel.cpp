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
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "GenericTopPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QSizePolicy>
#include <QToolButton>

namespace bonsaiviewer::components {

GenericTopPanel::GenericTopPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("genericTopPanel"));
    row_ = new QHBoxLayout(this);
    row_->setContentsMargins(2, 2, 2, 2);
    row_->setSpacing(0);
    // Equal stretch on the side zones: the centre zone stays in the middle
    // of the bar whatever the widths of the side groups.
    const auto zone = [this](int stretch) {
        auto* widget = new QWidget(this);
        auto* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        row_->addWidget(widget, stretch);
        return layout;
    };
    left_ = zone(1);
    centre_ = zone(0);
    right_ = zone(1);
    centre_->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void GenericTopPanel::setSearchBar(QWidget* bar) {
    if (!bar) return;
    // The left corner, before any group.
    left_->insertWidget(0, bar);
}

void GenericTopPanel::makeCompact() {
    // The strip never imposes its width: when the window is narrow, the
    // right-most groups are clipped instead of pushing the side panels.
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    setMinimumWidth(0);
    left_->addStretch(1);
    right_->insertStretch(0, 1);
    row_->setContentsMargins(2, 1, 2, 1);
    // Local sheet: overrides the ribbon button padding of Style.cpp.
    setStyleSheet(QStringLiteral("QToolButton#ribbonButton { padding: 2px; }"));

    for (auto* button : findChildren<QToolButton*>()) {
        if (button->toolTip().isEmpty()) button->setToolTip(button->text());
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIconSize(QSize(16, 16));
        button->setMinimumSize(QSize(24, 24));
        button->setMaximumHeight(24);
    }
    for (auto* label : findChildren<QLabel*>(QStringLiteral("ribbonGroupLabel"))) {
        label->hide();
    }
    for (auto* group : findChildren<QFrame*>(QStringLiteral("ribbonGroup"))) {
        if (group->layout()) group->layout()->setContentsMargins(4, 1, 4, 1);
    }
}

} // namespace bonsaiviewer::components
