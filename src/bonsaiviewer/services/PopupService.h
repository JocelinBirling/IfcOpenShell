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

#ifndef IFCINTERFACE_SERVICES_POPUPSERVICE_H
#define IFCINTERFACE_SERVICES_POPUPSERVICE_H

#include <QString>

class QWidget;

// B17 — PopupService (see 00_Technical_Implementation/BonsaiViewerTI00).
// The one place standard popups come from, so every feature shows the same
// title, wording pattern and iconography. TI02 ("The objects are far
// away"), TI03 ("No results") and the confirmation popups all go through
// these helpers.

namespace bonsaiviewer::services::popup {

// Information popup with a single OK button. Modal on `parent`.
void information(QWidget* parent, const QString& text);

// Question popup with OK / Cancel. True when the user confirms.
bool confirmation(QWidget* parent, const QString& text);

} // namespace bonsaiviewer::services::popup

#endif
