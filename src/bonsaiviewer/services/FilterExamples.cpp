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

#include "FilterExamples.h"

#include "IdsEngine.h"

#include <QUuid>

namespace bonsaiviewer::services {

namespace {

// An entity facet on one class, or on several (enumeration).
IdsFacet classes(const QStringList& names) {
    IdsFacet facet = IdsFacet::entity(names.first());
    if (names.size() > 1) {
        facet.name.kind = IdsValue::Kind::Enumeration;
        facet.name.values = names;
    }
    return facet;
}

// A property facet; an empty value = the property only has to exist.
IdsFacet property(const QString& pset, const QString& name, const QString& value = {},
                  const QString& data_type = {}) {
    IdsFacet facet = IdsFacet::propertyFacet();
    facet.pset = IdsValue::simpleValue(pset);
    facet.property = IdsValue::simpleValue(name);
    if (!value.isEmpty()) facet.value = IdsValue::simpleValue(value);
    facet.data_type = data_type;
    return facet;
}

// The property set of any class: Pset_WallCommon, Pset_SlabCommon...
IdsFacet commonProperty(const QString& name, const QString& value, const QString& data_type) {
    IdsFacet facet = property({}, name, value, data_type);
    facet.pset.kind = IdsValue::Kind::Pattern;
    facet.pset.simple.clear();
    facet.pset.pattern = "Pset_.*Common";
    return facet;
}

FilterLayer selectionLayer(const QString& name, const std::vector<IdsFacet>& facets,
                           std::vector<FilterAction> actions) {
    FilterLayer layer;
    layer.ids_xml = ids_engine::buildXml(name, facets);
    layer.actions = std::move(actions);
    return layer;
}

FilterLayer andLayer(std::vector<FilterAction> actions) {
    FilterLayer layer;
    layer.is_and = true;
    layer.actions = std::move(actions);
    return layer;
}

FilterAction action(FilterAction::Type type) {
    FilterAction result;
    result.type = type;
    return result;
}

FilterAction colour(const QColor& value) {
    FilterAction result = action(FilterAction::Type::Colour);
    result.colour = value;
    return result;
}

FilterAction transparency(int percent) {
    FilterAction result = action(FilterAction::Type::Transparency);
    result.transparency_percent = percent;
    return result;
}

FilterAction autoColour(const QString& pset, const QString& name) {
    FilterAction result = action(FilterAction::Type::AutoColourByProperty);
    result.pset = pset;
    result.property = name;
    return result;
}

FilterDocument document(const QString& folder_id, const QString& name, const QString& description,
                        std::vector<FilterLayer> layers) {
    FilterDocument result;
    result.id = QUuid::createUuid().toString();
    result.folder_id = folder_id;
    result.name = name;
    result.description = description;
    result.layers = std::move(layers);
    return result;
}

} // namespace

std::vector<FilterDocument> exampleFilters(const QString& folder_id) {
    using Type = FilterAction::Type;
    std::vector<FilterDocument> filters;

    filters.push_back(document(
        folder_id, "1. Isolate the walls",
        "The simplest filter: one layer, one entity facet (IfcWall and its subtypes), one action "
        "(Isolate). Run it with a double click or the Run button.",
        {selectionLayer("Walls", {IdsFacet::entity("IfcWall")}, {action(Type::Isolate)})}));

    filters.push_back(document(
        folder_id, "2. Hide slabs and roofs",
        "An entity facet on several classes (IfcSlab, IfcRoof) and the Remove from the view action: "
        "the floors and roofs disappear to look inside the building, the rest of the model stays as "
        "it is. Show All brings them back.",
        {selectionLayer("Slabs and roofs", {classes({"IfcSlab", "IfcRoof"})},
                        {action(Type::RemoveFromView)})}));

    filters.push_back(document(
        folder_id, "3. External walls in blue",
        "A property facet: Pset_WallCommon.IsExternal = true. Two layers run in order: the "
        "external walls are isolated and coloured in blue, then the internal walls are added to the "
        "view in grey.",
        {selectionLayer("External walls",
                        {IdsFacet::entity("IfcWall"), property("Pset_WallCommon", "IsExternal", "true", "IFCBOOLEAN")},
                        {action(Type::Isolate), colour(QColor(40, 110, 220))}),
         selectionLayer("Internal walls",
                        {IdsFacet::entity("IfcWall"), property("Pset_WallCommon", "IsExternal", "false", "IFCBOOLEAN")},
                        {action(Type::AddToView), colour(QColor(170, 170, 170))})}));

    filters.push_back(document(
        folder_id, "4. Load-bearing structure",
        "No entity facet: a selection can use any facet. The property set is a pattern "
        "(Pset_.*Common) so the walls, slabs, columns and beams are all found by their LoadBearing "
        "property. The structure is isolated in orange.",
        {selectionLayer("Load-bearing", {commonProperty("LoadBearing", "true", "IFCBOOLEAN")},
                        {action(Type::Isolate), colour(QColor(235, 140, 30))})}));

    filters.push_back(document(
        folder_id, "5. Doors and windows through the walls",
        "An AND layer reuses the elements of the layer above. The walls are isolated, then (AND) "
        "made 70 % transparent; the doors and windows are then added and coloured in green.",
        {selectionLayer("Walls", {IdsFacet::entity("IfcWall")}, {action(Type::Isolate)}),
         andLayer({transparency(70)}),
         selectionLayer("Doors and windows", {classes({"IfcDoor", "IfcWindow"})},
                        {action(Type::AddToView), colour(QColor(60, 180, 90))})}));

    filters.push_back(document(
        folder_id, "6. Colour the walls by type",
        "The Automatic Color action gives one colour per value of a property and opens a temporary "
        "table with the values and their colours. Here Pset_WallCommon.Reference, the wall type "
        "written by most authoring tools.",
        {selectionLayer("Walls", {IdsFacet::entity("IfcWall")},
                        {action(Type::Isolate), autoColour("Pset_WallCommon", "Reference")})}));

    filters.push_back(document(
        folder_id, "7. Load-bearing and partition walls",
        "Automatic Color on a true / false property (Pset_WallCommon.LoadBearing): two colours, "
        "load-bearing walls and partitions. The walls without the property are not coloured.",
        {selectionLayer("Walls", {IdsFacet::entity("IfcWall")},
                        {action(Type::Isolate), autoColour("Pset_WallCommon", "LoadBearing")})}));

    FilterAction by_level = action(Type::AutoColourByProperty);
    by_level.relation = "storey";
    filters.push_back(document(
        folder_id, "8. Colour by level",
        "Automatic Color by relation: the colour follows what the element is related to, here its "
        "level (IfcBuildingStorey). Other relations: model, building, space, system, zone, group, "
        "material, type, classification, class. An element with several relations (two systems, a "
        "layered material) gets the names joined.",
        {selectionLayer("Elements", {IdsFacet::entity("IfcElement")}, {by_level})}));

    return filters;
}

} // namespace bonsaiviewer::services
