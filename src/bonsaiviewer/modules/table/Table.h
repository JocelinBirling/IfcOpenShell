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

#ifndef IFCINTERFACE_MODULES_TABLE_TABLE_H
#define IFCINTERFACE_MODULES_TABLE_TABLE_H

#include "../../services/IdsBridge.h"

#include <QString>

#include <vector>

namespace bonsaiviewer::modules::table {

// TI07 — one column of a table.
struct TableColumn {
    enum class Kind {
        Property, // pset.property, from the property sets and quantity sets
        Name,
        Class,
        GlobalId,
        Model,
    };

    Kind kind = Kind::Property;
    QString title; // empty = automatic (the property name, or the kind)
    QString pset;
    QString property;
    // Texts that read as numbers (with "." or "," as decimal separator)
    // become real numbers.
    bool auto_numeric = true;

    QString displayTitle() const;
    static QString kindLabel(Kind kind);
};

// TI07 — a table: the elements of an IDS selection, evaluated on all the
// loaded models, one row per element, one column per field.
struct TableDefinition {
    QString name;
    QString description;
    bonsaiviewer::services::IdsBridge::SelectionFields selection;
    std::vector<TableColumn> columns;
};

inline QString TableColumn::kindLabel(Kind kind) {
    switch (kind) {
    case Kind::Property: return "Property";
    case Kind::Name: return "Name";
    case Kind::Class: return "Class";
    case Kind::GlobalId: return "GlobalId";
    case Kind::Model: return "Model";
    }
    return {};
}

inline QString TableColumn::displayTitle() const {
    if (!title.isEmpty()) return title;
    if (kind == Kind::Property) return property.isEmpty() ? QString("Property") : property;
    return kindLabel(kind);
}

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_TABLE_H
