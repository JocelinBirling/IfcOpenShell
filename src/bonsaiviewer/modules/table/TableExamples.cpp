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

#include "TableExamples.h"

namespace bonsaiviewer::modules::table {

namespace {

using bonsaiviewer::services::IdsFacet;
using bonsaiviewer::services::IdsValue;
using Kind = TableColumn::Kind;
using Aggregation = TableColumn::Aggregation;

TableColumn column(Kind kind) {
    TableColumn result;
    result.kind = kind;
    return result;
}

TableColumn property(const QString& pset, const QString& name,
                     Aggregation aggregation = Aggregation::Default) {
    TableColumn result = column(Kind::Property);
    result.pset = pset;
    result.property = name;
    result.aggregation = aggregation;
    return result;
}

TableColumn key(TableColumn base) {
    base.group_key = true;
    return base;
}

IdsFacet classes(const QStringList& names) {
    IdsFacet facet = IdsFacet::entity(names.first());
    if (names.size() > 1) {
        facet.name.kind = IdsValue::Kind::Enumeration;
        facet.name.values = names;
    }
    return facet;
}

TableDefinition table(const QString& name, const QString& description, std::vector<IdsFacet> facets,
                      std::vector<TableColumn> columns) {
    TableDefinition result;
    result.name = name;
    result.description = description;
    result.selection.facets = std::move(facets);
    result.columns = std::move(columns);
    return result;
}

} // namespace

std::vector<TableDefinition> exampleTables() {
    std::vector<TableDefinition> tables;

    tables.push_back(table(
        "1. Wall quantities",
        "One row per wall, evaluated on every loaded model. Columns: the name, the class, the model "
        "and quantities (Qto_WallBaseQuantities); texts that read as numbers become numbers. Click a "
        "row to select the wall, double click to isolate it, use the arrow and the funnel of a header "
        "to sort and filter, Export to save a CSV file.",
        {IdsFacet::entity("IfcWall")},
        {column(Kind::Name), column(Kind::Class), column(Kind::Model),
         property("Qto_WallBaseQuantities", "Length"), property("Qto_WallBaseQuantities", "NetSideArea"),
         property("Qto_WallBaseQuantities", "NetVolume")}));

    tables.push_back(table(
        "2. Elements per class",
        "A grouped table: the Class column is the grouping key, so the elements of one class form one "
        "row. Count gives the number of elements of each row; Color gives each row a colour, applied "
        "to its elements in the viewer.",
        {IdsFacet::entity("IfcElement")},
        {key(column(Kind::Class)), column(Kind::Count), column(Kind::Color)}));

    tables.push_back(table(
        "3. Wall areas by type",
        "Grouped on the wall's type (Pset_WallCommon.Reference) and on IsExternal. The areas and "
        "volumes are summed per row, the length is averaged: each column chooses its aggregation "
        "(Modify Column).",
        {IdsFacet::entity("IfcWall")},
        {key(property("Pset_WallCommon", "Reference")), key(property("Pset_WallCommon", "IsExternal")),
         column(Kind::Count), property("Qto_WallBaseQuantities", "NetSideArea", Aggregation::Sum),
         property("Qto_WallBaseQuantities", "NetVolume", Aggregation::Sum),
         property("Qto_WallBaseQuantities", "Length", Aggregation::Average)}));

    tables.push_back(table(
        "4. Doors and windows",
        "An entity facet on two classes (IfcDoor, IfcWindow). Run it on the box (Tables panel, right "
        "click) to list only the elements of the current box.",
        {classes({"IfcDoor", "IfcWindow"})},
        {column(Kind::Name), column(Kind::Class), column(Kind::GlobalId),
         property("Pset_DoorCommon", "FireRating"), property("Pset_DoorCommon", "IsExternal")}));

    tables.push_back(table(
        "5. Space areas",
        "The spaces with their net floor area (Qto_SpaceBaseQuantities). Sort on the area with the "
        "arrow of its header; filter the spaces of one model with the funnel of the Model column.",
        {IdsFacet::entity("IfcSpace")},
        {column(Kind::Name), property("Qto_SpaceBaseQuantities", "NetFloorArea"), column(Kind::Model)}));

    return tables;
}

} // namespace bonsaiviewer::modules::table
