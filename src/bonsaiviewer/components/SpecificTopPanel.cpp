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

#include "SpecificTopPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>

namespace bonsaiviewer::components {

SpecificTopPanel::SpecificTopPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("specificTopPanel"));
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    stack_ = new QStackedWidget(this);
    layout->addWidget(stack_);
    // Every page starts empty — the panel gives the room back to the 3D
    // view until a feature registers its content.
    hide();
}

int SpecificTopPanel::addTab(const QString& title) {
    auto* page = new QWidget(stack_);
    auto* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    // A hidden placeholder keeps the page minimal until a feature
    // registers its content; it never shows by itself.
    auto* label = new QLabel(title, page);
    label->hide();
    layout->addWidget(label);
    stack_->addWidget(page);
    return stack_->count() - 1;
}

void SpecificTopPanel::setContent(int index, QWidget* widget) {
    if (index < 0 || index >= stack_->count() || !widget) return;
    auto* page = stack_->widget(index);
    auto* layout = static_cast<QHBoxLayout*>(page->layout());
    if (layout->count() > 1) {
        // Replace a previously registered widget.
        QLayoutItem* old = layout->takeAt(1);
        delete old->widget();
        delete old;
    }
    widget->setParent(page);
    layout->addWidget(widget);
    // Registering content on the current page must reveal the panel
    // right away, not wait for the next tab switch.
    if (index == stack_->currentIndex()) {
        setVisible(true);
    }
}

void SpecificTopPanel::setCurrentIndex(int index) {
    if (index < 0 || index >= stack_->count()) return;
    stack_->setCurrentIndex(index);
    // An empty page collapses the whole panel.
    auto* page = stack_->widget(index);
    const auto* layout = page->layout();
    const bool has_content = layout->count() > 1;
    setVisible(has_content);
}

} // namespace bonsaiviewer::components
