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

#ifndef IFCINTERFACE_PANELS_PROPERTIESPANELTYPES_H
#define IFCINTERFACE_PANELS_PROPERTIESPANELTYPES_H

#include <QList>
#include <QPair>
#include <QPoint>
#include <QString>

#include <functional>

namespace bonsaiviewer::modules::properties {

struct KeyValueRow {
    QString key;
    QString value;
};

struct RelationshipRow {
    QString key;
    QString value;
};

struct PropertySet {
    QString title;
    QList<KeyValueRow> rows;
};

// TI14 — right click on a row of a property set: the set (as shown, under the
// filter), the row index in that set, whether the value (true) or the name
// (false) was clicked, and the global position.
using PropertyContextHandler = std::function<void(const PropertySet&, int, bool, const QPoint&)>;

struct EntitySummary {
    QString entity_class;
    QString predefined_type;
};

struct PropertiesPanelState {
    EntitySummary entity;
    QList<KeyValueRow> attributes;
    QList<RelationshipRow> relationships;
    QList<PropertySet> property_sets;
    QList<PropertySet> quantity_sets;
};

} // namespace bonsaiviewer::modules::properties

#endif
