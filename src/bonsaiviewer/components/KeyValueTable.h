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

#ifndef IFCINTERFACE_COMPONENTS_KEYVALUETABLE_H
#define IFCINTERFACE_COMPONENTS_KEYVALUETABLE_H

#include <QList>
#include <QPoint>
#include <QString>
#include <QWidget>

namespace bonsaiviewer::components {

struct KeyValueTableRow {
    QString key;
    QString value;
    QString value_object_name = "keyValueValueLabel";
    QString trailing_icon_path;
    QString trailing_icon_object_name;
    int key_minimum_width = 0;
    // The trailing icon is a button; a click emits linkActivated(row).
    bool link = false;
    // A text colour for the key and the value (e.g. an IDS result), and a
    // tooltip; empty = the default style.
    QString status_color;
    QString tooltip;
};

class KeyValueTable : public QWidget {
    Q_OBJECT
public:
    explicit KeyValueTable(const QList<KeyValueTableRow>& rows, QWidget* parent = nullptr);

signals:
    // Right click on the key (on_value = false) or the value (true) of a row.
    void contextMenuRequested(int row, bool on_value, const QPoint& global_pos);
    // Click on the trailing icon of a link row.
    void linkActivated(int row);
};

} // namespace bonsaiviewer::components

#endif
