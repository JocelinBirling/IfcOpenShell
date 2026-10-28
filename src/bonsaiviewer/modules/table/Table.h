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
        Count, // number of elements of the row (1 until grouping)
        Color, // a random colour per row, shown in the cell and in the viewer
        // What the element is related to (level, material...): `property`
        // holds the relation key of RelationValues. Used by the temporary
        // table of an Automatic Color by relation.
        Relation,
    };

    // How the values of the elements of a group are combined (TI07 part C).
    enum class Aggregation {
        Default,     // numbers: sum; texts: concatenation
        Concatenate, // all the values, as text
        Sum,
        Average,
        Min,
        Max,
        Count,       // number of non-empty values
    };

    Kind kind = Kind::Property;
    QString title; // empty = automatic (the property name, or the kind)
    QString pset;
    QString property;
    // Texts that read as numbers (with "." or "," as decimal separator)
    // become real numbers.
    bool auto_numeric = true;
    // Grouped table: the elements that share the values of the group-key
    // columns form one row. A group-key column shows that shared value.
    bool group_key = false;
    Aggregation aggregation = Aggregation::Default;

    QString displayTitle() const;
    static QString kindLabel(Kind kind);
    static QString aggregationLabel(Aggregation aggregation);
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
    case Kind::Count: return "Count";
    case Kind::Color: return "Color";
    case Kind::Relation: return "Relation";
    }
    return {};
}

inline QString TableColumn::aggregationLabel(Aggregation aggregation) {
    switch (aggregation) {
    case Aggregation::Default: return "Default";
    case Aggregation::Concatenate: return "Concatenate";
    case Aggregation::Sum: return "Sum";
    case Aggregation::Average: return "Average";
    case Aggregation::Min: return "Min";
    case Aggregation::Max: return "Max";
    case Aggregation::Count: return "Count";
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
