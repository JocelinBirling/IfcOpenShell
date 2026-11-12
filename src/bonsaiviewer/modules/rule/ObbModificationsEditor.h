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

#ifndef IFCINTERFACE_MODULES_RULE_OBBMODIFICATIONSEDITOR_H
#define IFCINTERFACE_MODULES_RULE_OBBMODIFICATIONSEDITOR_H

#include "ObbModifications.h"

#include <QJsonArray>
#include <QStringList>
#include <QWidget>

#include <vector>

class QLabel;
class QVBoxLayout;

namespace bonsaiviewer::modules::rule {

// BonsaiViewer09 (TI09) — the editor of the list_of_modifications of
// OBB_Custom (parameter kind "obb_modifications"): one row per step (an
// operation of the catalogue, a value, m or %, what it does), "New zone"
// separators, ↑ ↓ ✕ on each row; the zones are titled, the rows the engine
// would refuse are framed in orange with its message. An entry it cannot
// read (an older file) stays as text, in red, to correct or remove.
class ObbModificationsEditor : public QWidget {
    Q_OBJECT

public:
    // `operations`: the operation names of the catalogue (NEW_OBB included).
    ObbModificationsEditor(const QStringList& operations, const QJsonArray& entries, QWidget* parent = nullptr);

signals:
    // The list as entries, after every change.
    void changed(const QJsonArray& entries);

private:
    void rebuild();
    void moveRow(int index, int delta);
    void removeRow(int index);
    void emitChanged();
    QWidget* stepRow(int index);
    QWidget* zoneRow(int index, int zone);
    QWidget* rawRow(int index);
    QWidget* rowButtons(int index, QWidget* parent);

    QStringList operations_;
    std::vector<obb::Row> rows_;
    QVBoxLayout* list_ = nullptr;
    QLabel* message_ = nullptr;
};

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_OBBMODIFICATIONSEDITOR_H
