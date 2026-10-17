// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the License, or (at your option) any later            *
 * version.                                                                     *
 *                                                                              *
 ********************************************************************************/

#include "Catalogue.h"

#include <QJsonObject>
#include <QTreeWidgetItem>
#include <QUuid>

#include <mutex>

namespace bonsaiviewer::modules::rule {

namespace {

// The static mirror of the engine's registry (ifcclash_plus/registry.py),
// defined below: the fallback catalogue the UI uses before the engine
// answers, and the guarantee the panels never see an empty library.
std::vector<RuleDefinition> staticCatalogue();

// The catalogue installed by the engine; empty until the engine answers.
std::vector<RuleDefinition>& mutableCatalogue() {
    static std::vector<RuleDefinition> installed;
    static std::once_flag flag;
    std::call_once(flag, [] { installed = staticCatalogue(); });
    return installed;
}

RulePropertyDef floatParam(const QString& name, double default_value, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::Float;
    def.default_value = default_value;
    def.doc = doc;
    return def;
}

RulePropertyDef nullableFloatParam(const QString& name, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::NullableFloat;
    def.default_value = QString(); // empty = absent
    def.nullable = true;
    def.doc = doc;
    return def;
}

RulePropertyDef intParam(const QString& name, int default_value, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::Int;
    def.default_value = default_value;
    def.doc = doc;
    return def;
}

RulePropertyDef nullableIntParam(const QString& name, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::Int;
    def.default_value = QString();
    def.nullable = true;
    def.doc = doc;
    return def;
}

RulePropertyDef boolParam(const QString& name, bool default_value, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::Bool;
    def.default_value = default_value;
    def.doc = doc;
    return def;
}

RulePropertyDef textParam(const QString& name, const QString& default_value, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::Text;
    def.default_value = default_value;
    def.doc = doc;
    return def;
}

RulePropertyDef enumParam(const QString& name, const QStringList& values,
                          const QString& default_value, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::Enum;
    def.default_value = default_value;
    def.enum_values = values;
    def.doc = doc;
    return def;
}

RulePropertyDef directionParam(const QString& name, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::Direction;
    QVariantList direction;
    direction << 0.0 << 0.0 << 1.0;
    def.default_value = direction;
    def.doc = doc;
    return def;
}

RulePropertyDef stringListParam(const QString& name, const QString& doc = QString()) {
    RulePropertyDef def;
    def.name = name;
    def.type = RulePropertyType::StringList;
    def.default_value = QStringList();
    def.doc = doc;
    return def;
}

RuleDefinition makeRule(const QString& type, const QString& label, const QString& family,
                        const std::vector<QString>& slot_names, const QString& doc,
                        std::vector<RulePropertyDef> properties) {
    RuleDefinition definition;
    definition.name = type;
    definition.label = label;
    definition.family = family;
    definition.description = doc;
    definition.slot_names = slot_names;
    definition.properties = std::move(properties);
    return definition;
}

std::vector<RuleDefinition> staticCatalogue() {
    const std::vector<QString> no_slots;
    const std::vector<QString> source_slots = {QStringLiteral("source")};
    const std::vector<QString> source_target_slots = {QStringLiteral("source"), QStringLiteral("target")};
    const std::vector<QString> source_context_slots = {QStringLiteral("source"), QStringLiteral("context")};
    const std::vector<QString> full_slots = {
        QStringLiteral("source"), QStringLiteral("target"), QStringLiteral("context")};
    const QStringList wide_narrow = {QStringLiteral("Wide"), QStringLiteral("Narrow")};
    const QStringList cardinalities = {
        QStringLiteral("required"), QStringLiteral("optional"), QStringLiteral("prohibited")};
    const QString facets = QStringLiteral("IDS Facet");

    std::vector<RuleDefinition> catalogue = {
        makeRule("Volume", "Volume", "OneObjectRule", source_slots,
                 "Flags every source whose volume is outside [volume_min, volume_max].",
                 {floatParam("volume_min", 0.0), floatParam("volume_max", 100.0)}),
        makeRule("Area", "Area", "OneObjectRule", source_slots,
                 "Flags every source whose area is outside [volume_min, volume_max].",
                 {floatParam("volume_min", 0.0), floatParam("volume_max", 100.0)}),
        makeRule("TopOrBottomSurface", "Top Or Bottom Surface", "OneObjectRule", source_slots,
                 "Flags a source whose top (or bottom) face area is outside the bounds.",
                 {floatParam("surface_min", 0.0), floatParam("surface_max", 100.0),
                  enumParam("top_or_bot_method", {QStringLiteral("Top"), QStringLiteral("Bottom")},
                            QStringLiteral("Top"), "Which extreme face")}),
        makeRule("LateralSurface", "Lateral Surface", "OneObjectRule", source_slots,
                 "Flags a source whose lateral surface is outside the bounds.",
                 {floatParam("surface_min", 0.0), floatParam("surface_max", 100.0),
                  directionParam("direction", "Measurement direction")}),
        makeRule("ProjectedSurface", "Projected Surface", "OneObjectRule", source_slots,
                 "Flags a source whose projected surface is outside the bounds.",
                 {floatParam("surface_min", 0.0), floatParam("surface_max", 100.0),
                  directionParam("direction", "Projection direction")}),
        makeRule("ObbHigh", "Obb High", "OneObjectRule", source_slots,
                 "Flags a source whose OBB height is outside [length_min, length_max].",
                 {floatParam("length_min", 0.0), floatParam("length_max", 100.0)}),
        makeRule("ObbLength", "Obb Length", "OneObjectRule", source_slots,
                 "Flags a source whose OBB length is outside [length_min, length_max].",
                 {floatParam("length_min", 0.0), floatParam("length_max", 100.0),
                  enumParam("direction_method", wide_narrow, QStringLiteral("Wide"), "OBB axis choice")}),
        makeRule("Orientation", "Orientation", "OneObjectRule", source_slots,
                 "Flags a source whose orientation deviates from the reference direction.",
                 {directionParam("orientation", "Reference direction"),
                  enumParam("orientation_type", {QStringLiteral("Parrallel"), QStringLiteral("Perpendicular")},
                            QStringLiteral("Parrallel"), "Relation to the direction"),
                  enumParam("direction_method", wide_narrow, QStringLiteral("Wide"), "OBB axis choice"),
                  floatParam("angular_tolerance", 0.1, "Angular tolerance")}),
        makeRule("OneObjectFace", "One Object Face", "OneObjectRule", source_slots,
                 "Face check of one element against itself (distance, intersection, orientation).",
                 {textParam("face_a_selection", QString(), "First face selection (empty = all)"),
                  textParam("face_b_selection", QString(), "Second face selection (empty = all)"),
                  boolParam("distance", false, "Measure the distance"),
                  nullableFloatParam("min_distance", "Minimum distance"),
                  nullableFloatParam("max_distance", "Maximum distance"),
                  boolParam("skip_adjacent", true, "Skip adjacent faces"),
                  boolParam("intersection", false, "Check the intersection"),
                  nullableFloatParam("intersection_tolerance", "Intersection tolerance"),
                  boolParam("orientation", false, "Check the orientation"),
                  nullableFloatParam("angle", "Angle"),
                  nullableFloatParam("angle_tolerance", "Angle tolerance")}),
        makeRule("AngleBetween", "Angle Between", "TwoObjectRule", source_target_slots,
                 "Flags the pairs whose angle between the source and target axes deviates.",
                 {enumParam("direction_method_for_source", wide_narrow, QStringLiteral("Wide"), "Source OBB axis"),
                  enumParam("direction_method_for_target", wide_narrow, QStringLiteral("Wide"), "Target OBB axis"),
                  floatParam("angle_difference", 0.0, "Expected angle difference"),
                  floatParam("angle_tolerance", 0.0, "Angle tolerance")}),
        makeRule("Intersection", "Intersection", "TwoObjectRule", source_target_slots,
                 "Flags the pairs whose shapes intersect within the tolerance.",
                 {floatParam("tolerance", 0.1, "Intersection tolerance")}),
        makeRule("Clearance", "Clearance", "TwoObjectRule", source_target_slots,
                 "Flags the pairs closer than the clearance.",
                 {floatParam("clearance", 0.05, "Minimum required clearance"),
                  boolParam("check_all", false, "Check all the pairs, not only the closest")}),
        makeRule("ClearanceAxisToAxis", "Clearance Axis To Axis", "TwoObjectRule", source_target_slots,
                 "Flags the pairs of linear elements whose axis-to-axis distance is outside the bounds.",
                 {enumParam("mode", {QStringLiteral("Perpendicular"), QStringLiteral("MinDistance")},
                            QStringLiteral("Perpendicular"), "Distance mode"),
                  nullableFloatParam("min_distance", "Minimum distance"),
                  nullableFloatParam("max_distance", "Maximum distance"),
                  floatParam("tolerance", 0.0, "Distance tolerance"),
                  floatParam("linearity_ratio", 10.0, "Linearity ratio"),
                  enumParam("on_non_linear", {QStringLiteral("Skip"), QStringLiteral("Fallback")},
                            QStringLiteral("Skip"), "Non linear elements"),
                  floatParam("parallel_tolerance", 1.0, "Parallelism tolerance")}),
        makeRule("Collision", "Collision", "TwoObjectRule", source_target_slots,
                 "Flags the pairs of colliding shapes; touching may be allowed.",
                 {boolParam("allow_touching", false, "Allow face contact")}),
        makeRule("Above", "Above", "TwoObjectRule", source_target_slots,
                 "Flags the pairs whose source is not above the target, per the bounding method.",
                 {enumParam("above_type", {QStringLiteral("Above_MinToMax"), QStringLiteral("Above_MinToMin"),
                                           QStringLiteral("Above_MaxToMin"), QStringLiteral("Above_MaxToMax")},
                            QStringLiteral("Above_MaxToMax"), "Bounding method"),
                  floatParam("tolerance", 0.1, "Tolerance")}),
        makeRule("Below", "Below", "TwoObjectRule", source_target_slots,
                 "Flags the pairs whose source is not below the target, per the bounding method.",
                 {enumParam("below_type", {QStringLiteral("Below_MinToMax"), QStringLiteral("Below_MinToMin"),
                                           QStringLiteral("Below_MaxToMin"), QStringLiteral("Below_MaxToMax")},
                            QStringLiteral("Below_MaxToMax"), "Bounding method"),
                  floatParam("tolerance", 0.1, "Tolerance")}),
        makeRule("OBB_Above", "OBB Above", "TwoObjectRule", source_target_slots,
                 "Flags a target penetrating the detection band extruded above the source OBB.",
                 {floatParam("tolerance", 1.0, "Height of the detection band above the source")}),
        makeRule("OBB_Below", "OBB Below", "TwoObjectRule", source_target_slots,
                 "Flags a target penetrating the detection band extruded below the source OBB.",
                 {floatParam("tolerance", 1.0, "Height of the detection band below the source")}),
        makeRule("OBB_Front_And_Back", "OBB Front And Back", "TwoObjectRule", source_target_slots,
                 "Flags a target penetrating the detection bands in front of and behind the source.",
                 {floatParam("tolerance", 0.1, "Depth of the detection bands"),
                  enumParam("direction_method", wide_narrow, QStringLiteral("Wide"), "Front direction")}),
        makeRule("OBB_Custom", "OBB Custom", "TwoObjectRule", source_target_slots,
                 "Flags a target penetrating custom detection zones around the source.",
                 {stringListParam("list_of_modifications",
                                  "Zone modifications, one per line (ex. 'top 0.5', 'bottom 0.2')")}),
        makeRule("SurfaceRecover", "Surface Recover", "TwoObjectRule", source_target_slots,
                 "Checks that the target surface is covered by the source (ex. a coating).",
                 {enumParam("direction", {QStringLiteral("Auto"), QStringLiteral("Top"), QStringLiteral("Bottom")},
                            QStringLiteral("Auto"), "Recovery direction"),
                  enumParam("reference", {QStringLiteral("Source"), QStringLiteral("Target")},
                            QStringLiteral("Source"), "Reference surface"),
                  floatParam("tolerance", 0.001, "Tolerance")}),
        makeRule("TwoObjectsFace", "Two Objects Face", "TwoObjectRule", source_target_slots,
                 "Face check of the source faces against the target faces.",
                 {textParam("source_face_selection", QString(), "Source face selection (empty = all)"),
                  textParam("target_face_selection", QString(), "Target face selection (empty = all)"),
                  boolParam("distance", false, "Measure the distance"),
                  nullableFloatParam("min_distance", "Minimum distance"),
                  nullableFloatParam("max_distance", "Maximum distance"),
                  boolParam("intersection", false, "Check the intersection"),
                  nullableFloatParam("intersection_tolerance", "Intersection tolerance"),
                  boolParam("orientation", false, "Check the orientation"),
                  nullableFloatParam("angle", "Angle"),
                  nullableFloatParam("angle_tolerance", "Angle tolerance")}),
        makeRule("ClearanceForDoors", "Clearance For Doors", "TwoObjectRule", source_target_slots,
                 "Checks that the door swing clearance zone is free of target obstacles.",
                 {enumParam("zone_shape", {QStringLiteral("Arc"), QStringLiteral("Rectangle")},
                            QStringLiteral("Arc"), "Zone shape"),
                  enumParam("sides", {QStringLiteral("Swing"), QStringLiteral("Both"),
                                      QStringLiteral("Front"), QStringLiteral("Back")},
                            QStringLiteral("Swing"), "Which side(s)"),
                  nullableIntParam("leaves", "Number of door leaves (empty = auto)"),
                  nullableFloatParam("width", "Zone width (empty = door width)"),
                  nullableFloatParam("depth", "Zone depth (empty = door width)"),
                  nullableFloatParam("height", "Zone height (empty = door height)"),
                  floatParam("tolerance", 0.001, "Tolerance")}),
        makeRule("Alignement", "Alignement", "ComplexRule", source_slots,
                 "Flags the groups of sources aligned on a common axis or plane.",
                 {enumParam("axis", {QString(), QStringLiteral("Vertical"), QStringLiteral("Horizontal")},
                            QString(), "Axis (empty = auto)"),
                  enumParam("alignment_type", {QStringLiteral("Line"), QStringLiteral("Plane")},
                            QStringLiteral("Plane"), "Alignment type"),
                  nullableFloatParam("tolerance", "Tolerance (empty = auto)"),
                  intParam("min_group", 3, "Minimum group size")}),
        makeRule("Ray_Check", "Ray Check", "ComplexRule", full_slots,
                 "Casts rays from the sources to the context obstacles.",
                 {floatParam("max_ray_length", 1.0, "Maximum ray length")}),
        makeRule("DirectView", "Direct View", "ComplexRule", full_slots,
                 "Flags the source/target pairs without a direct line of sight through the context.",
                 {enumParam("ray_source", {QStringLiteral("Source"), QStringLiteral("Target"),
                                           QStringLiteral("Both")},
                            QStringLiteral("Source"), "Where the rays start"),
                  intParam("ray_count", 10, "Number of rays per pair"),
                  floatParam("threshold", 100.0, "Visibility threshold"),
                  nullableFloatParam("max_distance", "Maximum distance (empty = none)")}),
        makeRule("FreeSpace", "Free Space", "ComplexRule", source_context_slots,
                 "Checks that a free cylinder (ex. evacuation) fits around each source.",
                 {floatParam("diameter", 1.5, "Free space diameter"),
                  floatParam("height", 2.0, "Free space height")}),
        // IDS facets: parameters mirror the ifctester facet attributes; the
        // complex-capable values carry an Enumeration/Pattern/Bounds/Length
        // restriction block (SPEC_GeometryRule_UI.md 5).
        makeRule("Entity", "Entity Facet", facets, no_slots,
                 "Selects the elements by IFC class and predefined type.",
                 {textParam("name", QStringLiteral("IFCWALL"), "IFC class name"),
                  textParam("predefinedType", QString(), "Predefined type"),
                  textParam("instructions", QString(), "Instructions")}),
        makeRule("Attribute", "Attribute Facet", facets, no_slots,
                 "Filters the elements by an attribute value.",
                 {textParam("name", QStringLiteral("Name"), "Attribute name"),
                  textParam("value", QString(), "Attribute value"),
                  enumParam("cardinality", cardinalities, QStringLiteral("required"), "Cardinality"),
                  textParam("instructions", QString(), "Instructions")}),
        makeRule("Property", "Property Facet", facets, no_slots,
                 "Filters the elements by a property value.",
                 {textParam("propertySet", QStringLiteral("Property_Set"), "Property set"),
                  textParam("baseName", QStringLiteral("PropertyName"), "Property name"),
                  textParam("value", QString(), "Property value"),
                  textParam("dataType", QString(), "Data type"),
                  enumParam("cardinality", cardinalities, QStringLiteral("required"), "Cardinality"),
                  textParam("instructions", QString(), "Instructions")}),
        makeRule("Classification", "Classification Facet", facets, no_slots,
                 "Filters the elements by classification reference.",
                 {textParam("value", QString(), "Classification value"),
                  textParam("system", QString(), "Classification system"),
                  enumParam("cardinality", cardinalities, QStringLiteral("required"), "Cardinality"),
                  textParam("instructions", QString(), "Instructions")}),
        makeRule("Material", "Material Facet", facets, no_slots,
                 "Filters the elements by material.",
                 {textParam("value", QString(), "Material value"),
                  enumParam("cardinality", cardinalities, QStringLiteral("required"), "Cardinality"),
                  textParam("instructions", QString(), "Instructions")}),
        makeRule("PartOf", "PartOf Facet", facets, no_slots,
                 "Filters the elements by a relationship to another element.",
                 {textParam("name", QStringLiteral("IFCWALL"), "Related element class"),
                  textParam("predefinedType", QString(), "Related predefined type"),
                  textParam("relation", QString(), "Relation"),
                  enumParam("cardinality", cardinalities, QStringLiteral("required"), "Cardinality"),
                  textParam("instructions", QString(), "Instructions")}),
    };
    return catalogue;
}

} // namespace

const std::vector<RuleDefinition>& ruleCatalogue() {
    return mutableCatalogue();
}

const RuleDefinition* findRuleDefinition(const QString& name) {
    for (const auto& definition : ruleCatalogue()) {
        if (definition.name == name) return &definition;
    }
    return nullptr;
}

const RuleDefinition* resolveRuleDefinition(const QString& text) {
    if (const RuleDefinition* definition = findRuleDefinition(text)) return definition;
    for (const auto& definition : ruleCatalogue()) {
        if (definition.label == text) return &definition;
    }
    return nullptr;
}

int setCatalogueFromEngine(const QJsonArray& rules, const QJsonArray& facets) {
    std::vector<RuleDefinition> parsed;

    const auto parse_params = [](const QJsonArray& params) {
        std::vector<RulePropertyDef> properties;
        for (const QJsonValue& value : params) {
            const QJsonObject param = value.toObject();
            RulePropertyDef def;
            def.name = param.value("name").toString();
            def.doc = param.value("doc").toString();
            def.unit = param.value("unit").toString();
            const QString kind = param.value("kind").toString();
            const QJsonValue raw_default = param.value("default");
            const bool complex = param.value("complex").toBool();
            if (kind == QStringLiteral("float")) {
                if (raw_default.isNull()) {
                    def.type = RulePropertyType::NullableFloat;
                    def.nullable = true;
                    def.default_value = QString();
                } else {
                    def.type = RulePropertyType::Float;
                    def.default_value = raw_default.toDouble();
                }
            } else if (kind == QStringLiteral("int")) {
                def.type = RulePropertyType::Int;
                if (raw_default.isNull()) {
                    def.nullable = true;
                    def.default_value = QString();
                } else {
                    def.default_value = raw_default.toInt();
                }
            } else if (kind == QStringLiteral("bool")) {
                def.type = RulePropertyType::Bool;
                def.default_value = raw_default.toBool();
            } else if (kind == QStringLiteral("enum")) {
                def.type = RulePropertyType::Enum;
                def.default_value = raw_default.toString();
                for (const QJsonValue& choice : param.value("values").toArray()) {
                    def.enum_values.append(choice.toString());
                }
            } else if (kind == QStringLiteral("string_list")) {
                def.type = RulePropertyType::StringList;
                def.default_value = QStringList();
            } else if (kind == QStringLiteral("direction")) {
                def.type = RulePropertyType::Direction;
                QVariantList direction;
                for (const QJsonValue& axis : raw_default.toArray()) direction.append(axis.toDouble());
                def.default_value = direction;
            } else { // string
                def.type = RulePropertyType::Text;
                def.default_value = raw_default.toString();
            }
            if (complex) def.doc += QStringLiteral(" [complex]");
            properties.push_back(std::move(def));
        }
        return properties;
    };

    const auto parse_entry = [&](const QJsonObject& entry, const QString& family_override) {
        RuleDefinition definition;
        definition.name = entry.value("type").toString();
        definition.label = entry.value("label").toString();
        definition.family = family_override.isEmpty() ? entry.value("family").toString() : family_override;
        definition.description = entry.value("doc").toString();
        for (const QJsonValue& slot : entry.value("slots").toArray()) {
            definition.slot_names.push_back(slot.toString());
        }
        definition.properties = parse_params(entry.value("params").toArray());
        return definition;
    };

    for (const QJsonValue& value : rules) parsed.push_back(parse_entry(value.toObject(), QString()));
    for (const QJsonValue& value : facets) parsed.push_back(parse_entry(value.toObject(), QString()));

    if (parsed.empty()) return 0;
    mutableCatalogue() = std::move(parsed);
    return static_cast<int>(ruleCatalogue().size());
}

void initRuleInstance(QTreeWidgetItem* item, const RuleDefinition& definition) {
    QVariantMap values;
    for (const auto& property : definition.properties) {
        values.insert(property.name, property.default_value);
    }
    item->setData(0, kRuleKeyRole, definition.name);
    item->setData(0, kRuleValuesRole, values);
    item->setData(0, kDescriptionRole, definition.description);
    if (item->data(0, kNodeIdRole).toString().isEmpty()) {
        item->setData(0, kNodeIdRole, QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
}

} // namespace bonsaiviewer::modules::rule
