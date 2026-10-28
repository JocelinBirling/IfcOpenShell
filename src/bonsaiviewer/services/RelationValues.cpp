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

#include "RelationValues.h"

#include "../ports/ifcopenshell_util/Classification.h"
#include "../ports/ifcopenshell_util/Element.h"

#include <QStringList>

#include <set>

namespace bonsaiviewer::services::relations {

namespace {

namespace util = bonsaiviewer::ports::ifcopenshell_util;
using Entity = util::Entity;

QString text(const std::optional<std::string>& value) {
    return value ? QString::fromStdString(*value).trimmed() : QString();
}

// The Name of an entity, else its LongName (spaces, zones).
QString nameOf(const Entity& entity) {
    if (!entity) return {};
    QString name = text(util::forwardString(entity, "Name"));
    if (name.isEmpty() && util::attributeCategory(entity, "LongName") == 1) {
        name = text(util::forwardString(entity, "LongName"));
    }
    return name;
}

void insert(std::set<QString>& names, const QString& name) {
    if (!name.isEmpty()) names.insert(name);
}

// The spatial elements of class `ifc_class` holding the element: its
// container (walking up the aggregation) and, IFC4, the structures it is
// referenced in.
void spatial(const Entity& element, const std::string& ifc_class, std::set<QString>& names) {
    insert(names, nameOf(util::get_container(element, false, ifc_class)));
    for (const Entity& rel : util::inverse(element, "ReferencedInStructures")) {
        Entity structure = util::forwardEntity(rel, "RelatingStructure");
        while (structure && !util::isA(structure, ifc_class)) structure = util::get_aggregate(structure);
        insert(names, nameOf(structure));
    }
}

// The groups the element is assigned to, kept by `keep`.
template <typename Keep>
void groups(const Entity& element, Keep keep, std::set<QString>& names) {
    for (const Entity& rel : util::inverse(element, "HasAssignments")) {
        if (!util::isA(rel, "IfcRelAssignsToGroup")) continue;
        const Entity group = util::forwardEntity(rel, "RelatingGroup");
        if (group && keep(group)) insert(names, nameOf(group));
    }
}

bool isZone(const Entity& group) {
    return util::isA(group, "IfcZone");
}

bool isSystem(const Entity& group) {
    return util::isA(group, "IfcSystem") && !isZone(group);
}

bool isOtherGroup(const Entity& group) {
    return !util::isA(group, "IfcSystem") && !isZone(group);
}

// A material, or the materials of a set (layers, profiles, constituents, list).
void materials(const Entity& material, std::set<QString>& names) {
    if (!material) return;
    const auto parts = [&](const char* list, const char* item) {
        for (const Entity& part : util::forwardEntities(material, list)) {
            insert(names, nameOf(item ? util::forwardEntity(part, item) : part));
        }
    };
    if (util::isA(material, "IfcMaterial")) insert(names, nameOf(material));
    else if (util::isA(material, "IfcMaterialLayerSet")) parts("MaterialLayers", "Material");
    else if (util::isA(material, "IfcMaterialProfileSet")) parts("MaterialProfiles", "Material");
    else if (util::isA(material, "IfcMaterialConstituentSet")) parts("MaterialConstituents", "Material");
    else if (util::isA(material, "IfcMaterialList")) parts("Materials", nullptr);
    else if (util::isA(material, "IfcMaterialLayer") || util::isA(material, "IfcMaterialProfile")) {
        insert(names, nameOf(util::forwardEntity(material, "Material")));
    }
}

void classifications(const Entity& element, std::set<QString>& names) {
    for (const Entity& reference : util::get_references(element)) {
        QString id = util::attributeCategory(reference, "Identification") == 1
                         ? text(util::forwardString(reference, "Identification"))
                         : text(util::forwardString(reference, "ItemReference"));
        insert(names, id.isEmpty() ? nameOf(reference) : id);
    }
}

} // namespace

const std::vector<Relation>& all() {
    static const std::vector<Relation> relations = {
        {"model", "Model"},         {"storey", "Level"},
        {"building", "Building"},   {"space", "Space"},
        {"system", "System"},       {"zone", "Zone"},
        {"group", "Group"},         {"material", "Material"},
        {"type", "Type"},           {"classification", "Classification"},
        {"class", "Class"},
    };
    return relations;
}

QString label(const QString& key) {
    for (const Relation& relation : all()) {
        if (relation.key == key) return relation.label;
    }
    return key;
}

QString value(const Entity& element, const QString& key, const QString& model_name) {
    if (key == QLatin1String("model")) return model_name;
    if (!element) return {};
    std::set<QString> names;
    try {
        if (key == QLatin1String("storey")) spatial(element, "IfcBuildingStorey", names);
        else if (key == QLatin1String("building")) spatial(element, "IfcBuilding", names);
        else if (key == QLatin1String("space")) spatial(element, "IfcSpace", names);
        else if (key == QLatin1String("system")) groups(element, isSystem, names);
        else if (key == QLatin1String("group")) groups(element, isOtherGroup, names);
        else if (key == QLatin1String("zone")) {
            groups(element, isZone, names);
            // Elements are rarely in a zone: the zones of their space.
            if (names.empty()) {
                if (const Entity space = util::get_container(element, false, "IfcSpace")) {
                    groups(space, isZone, names);
                }
            }
        } else if (key == QLatin1String("material")) materials(util::get_material(element, true), names);
        else if (key == QLatin1String("type")) insert(names, nameOf(util::get_type(element)));
        else if (key == QLatin1String("classification")) classifications(element, names);
        else if (key == QLatin1String("class")) insert(names, QString::fromStdString(util::className(element)));
    } catch (...) {
        return {}; // an attribute the schema of the file does not have
    }
    QStringList joined;
    for (const QString& name : names) joined.append(name);
    return joined.join(", ");
}

} // namespace bonsaiviewer::services::relations
