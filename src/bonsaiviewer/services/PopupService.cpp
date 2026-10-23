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

#include "PopupService.h"

#include <QMessageBox>

namespace bonsaiviewer::services::popup {

void information(QWidget* parent, const QString& text) {
    QMessageBox::information(parent, QStringLiteral("Bonsai Viewer"), text);
}

bool confirmation(QWidget* parent, const QString& text) {
    return QMessageBox::question(parent, QStringLiteral("Bonsai Viewer"), text,
                                 QMessageBox::Ok | QMessageBox::Cancel,
                                 QMessageBox::Cancel) == QMessageBox::Ok;
}

} // namespace bonsaiviewer::services::popup
