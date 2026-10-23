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

namespace bonsaiviewer::components {

GenericTopPanel::GenericTopPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("genericTopPanel"));
    row_ = new QHBoxLayout(this);
    row_->setContentsMargins(2, 2, 2, 2);
    row_->setSpacing(0);
}

void GenericTopPanel::setSearchBar(QWidget* bar) {
    if (!bar) return;
    // The left corner, before any group.
    row_->insertWidget(0, bar);
}

} // namespace bonsaiviewer::components
