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

#include "IdsBridge.h"

#include "IdsEngine.h"

#include "../ifcparse/file.h"

#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QPointer>
#include <QTimer>

#include <utility>

namespace bonsaiviewer::services {

namespace {

QString kindName(IdsValue::Kind kind) {
    switch (kind) {
    case IdsValue::Kind::Any: return "any";
    case IdsValue::Kind::Simple: return "simple";
    case IdsValue::Kind::Enumeration: return "enumeration";
    case IdsValue::Kind::Pattern: return "pattern";
    case IdsValue::Kind::Range: return "range";
    case IdsValue::Kind::Length: return "length";
    }
    return "any";
}

const char* typeKey(IdsFacet::Type type) {
    switch (type) {
    case IdsFacet::Type::Entity: return "entity";
    case IdsFacet::Type::Property: return "property";
    case IdsFacet::Type::Attribute: return "attribute";
    case IdsFacet::Type::Classification: return "classification";
    case IdsFacet::Type::Material: return "material";
    case IdsFacet::Type::PartOf: return "partOf";
    }
    return "entity";
}

// "Simple" text of a name-like value: the value, /pattern/, {a|b}...
QString nameOf(const IdsValue& value) {
    switch (value.kind) {
    case IdsValue::Kind::Simple: return value.simple;
    case IdsValue::Kind::Pattern: return QString("/%1/").arg(value.pattern);
    case IdsValue::Kind::Enumeration: return QString("{%1}").arg(value.values.join("|"));
    case IdsValue::Kind::Length: return QString("(%1)").arg(value.summary());
    case IdsValue::Kind::Any:
    case IdsValue::Kind::Range: break;
    }
    return QStringLiteral("?");
}

QString classLabel(const IdsFacet& facet) {
    QString label;
    switch (facet.name.kind) {
    case IdsValue::Kind::Simple: label = facet.name.simple; break;
    case IdsValue::Kind::Enumeration: label = QString("{%1}").arg(facet.name.values.join(", ")); break;
    case IdsValue::Kind::Pattern: label = QString("/%1/").arg(facet.name.pattern); break;
    default: label = QStringLiteral("(no class)"); break;
    }
    if (facet.include_subtypes) label += "+";
    if (!facet.predefined_type.isAny()) {
        label += facet.predefined_type.kind == IdsValue::Kind::Simple
            ? QString(" / %1").arg(facet.predefined_type.simple)
            : QString(" / %1").arg(facet.predefined_type.summary());
    }
    return label;
}

// The value part of a summary: " = x", " exists"...
QString valuePart(const IdsValue& value) {
    return value.isAny() ? QStringLiteral(" exists") : " " + value.summary();
}

QString withCardinality(const QString& label, const QString& cardinality) {
    if (cardinality == "prohibited") return "no " + label;
    if (cardinality == "optional") return label + " (optional)";
    return label;
}

} // namespace

// ---------------------------------------------------------------------------
// IdsValue / IdsFacet
// ---------------------------------------------------------------------------

IdsValue IdsValue::simpleValue(const QString& text) {
    IdsValue value;
    value.kind = text.isEmpty() ? Kind::Any : Kind::Simple;
    value.simple = text;
    return value;
}

QString IdsValue::summary() const {
    switch (kind) {
    case Kind::Any:
        return {};
    case Kind::Simple:
        return QString("= %1").arg(simple);
    case Kind::Enumeration: {
        QStringList shown = values.mid(0, 3);
        if (values.size() > 3) shown << QString("+%1").arg(values.size() - 3);
        return QString("∈ {%1}").arg(shown.join(", "));
    }
    case Kind::Pattern:
        return QString("~ /%1/").arg(pattern);
    case Kind::Range: {
        if (!min.isEmpty() && !max.isEmpty()) {
            return QString("∈ %1%2 ; %3%4")
                .arg(min_inclusive ? "[" : "]", min, max, max_inclusive ? "]" : "[");
        }
        if (!min.isEmpty()) return QString("%1 %2").arg(min_inclusive ? "≥" : ">", min);
        if (!max.isEmpty()) return QString("%1 %2").arg(max_inclusive ? "≤" : "<", max);
        return {};
    }
    case Kind::Length:
        if (!min.isEmpty() && min == max) return QString("length = %1").arg(min);
        if (!min.isEmpty() && !max.isEmpty()) return QString("length %1..%2").arg(min, max);
        if (!min.isEmpty()) return QString("length ≥ %1").arg(min);
        if (!max.isEmpty()) return QString("length ≤ %1").arg(max);
        return {};
    }
    return {};
}

QJsonValue IdsValue::toJson() const {
    QJsonObject json;
    json["kind"] = kindName(kind);
    switch (kind) {
    case Kind::Any:
        return QJsonValue::Null;
    case Kind::Simple:
        if (simple.trimmed().isEmpty()) return QJsonValue::Null;
        json["value"] = simple.trimmed();
        break;
    case Kind::Enumeration:
        json["values"] = QJsonArray::fromStringList(values);
        break;
    case Kind::Pattern:
        json["pattern"] = pattern;
        break;
    case Kind::Range:
        json["min"] = min.trimmed();
        json["max"] = max.trimmed();
        json["minInclusive"] = min_inclusive;
        json["maxInclusive"] = max_inclusive;
        json["base"] = base;
        break;
    case Kind::Length:
        json["min"] = min.trimmed();
        json["max"] = max.trimmed();
        break;
    }
    return json;
}

IdsValue IdsValue::fromJson(const QJsonValue& json) {
    IdsValue value;
    if (!json.isObject()) return value;
    const QJsonObject object = json.toObject();
    const QString kind = object["kind"].toString("simple");
    if (kind == "simple") {
        value.kind = Kind::Simple;
        value.simple = object["value"].toString();
    } else if (kind == "enumeration") {
        value.kind = Kind::Enumeration;
        for (const auto& entry : object["values"].toArray()) value.values << entry.toString();
    } else if (kind == "pattern") {
        value.kind = Kind::Pattern;
        value.pattern = object["pattern"].toString();
    } else if (kind == "range") {
        value.kind = Kind::Range;
        value.min = object["min"].toString();
        value.max = object["max"].toString();
        value.min_inclusive = object["minInclusive"].toBool(true);
        value.max_inclusive = object["maxInclusive"].toBool(true);
        value.base = object["base"].toString("double");
    } else if (kind == "length") {
        value.kind = Kind::Length;
        value.min = object["min"].toString();
        value.max = object["max"].toString();
    }
    return value;
}

IdsFacet IdsFacet::entity(const QString& ifc_class) {
    IdsFacet facet;
    facet.type = Type::Entity;
    facet.name = IdsValue::simpleValue(ifc_class);
    facet.include_subtypes = true;
    return facet;
}

IdsFacet IdsFacet::propertyFacet() {
    return ofType(Type::Property);
}

IdsFacet IdsFacet::ofType(Type type) {
    IdsFacet facet;
    facet.type = type;
    facet.include_subtypes = type == Type::Entity || type == Type::PartOf;
    return facet;
}

QString IdsFacet::typeLabel(Type type) {
    switch (type) {
    case Type::Entity: return "Entity";
    case Type::Property: return "Property";
    case Type::Attribute: return "Attribute";
    case Type::Classification: return "Classification";
    case Type::Material: return "Material";
    case Type::PartOf: return "Part of";
    }
    return {};
}

QString IdsFacet::summary() const {
    switch (type) {
    case Type::Entity:
        return classLabel(*this);
    case Type::Property: {
        QString label = QString("%1.%2").arg(nameOf(pset), nameOf(property));
        if (cardinality != "prohibited") label += valuePart(value);
        if (!data_type.isEmpty()) label += QString(" [%1]").arg(data_type);
        return withCardinality(label, cardinality);
    }
    case Type::Attribute: {
        QString label = nameOf(property);
        if (cardinality != "prohibited") label += valuePart(value);
        return withCardinality(label, cardinality);
    }
    case Type::Classification: {
        QString label = system.isAny() ? QStringLiteral("classification") : nameOf(system);
        label += value.isAny() ? QStringLiteral(" (any reference)") : " " + value.summary();
        return withCardinality(label, cardinality);
    }
    case Type::Material: {
        const QString label = value.isAny() ? QStringLiteral("material") : "material " + value.summary();
        return withCardinality(label, cardinality);
    }
    case Type::PartOf: {
        QString label = "part of " + classLabel(*this);
        if (!relation.isEmpty()) label += QString(" (%1)").arg(relation.toLower().remove("ifcrel"));
        return withCardinality(label, cardinality);
    }
    }
    return {};
}

QJsonObject IdsFacet::toJson() const {
    QJsonObject json;
    json["type"] = typeKey(type);
    switch (type) {
    case Type::Entity:
        json["name"] = name.toJson();
        json["includeSubtypes"] = include_subtypes;
        json["predefinedType"] = predefined_type.toJson();
        if (!instructions.isEmpty()) json["instructions"] = instructions;
        return json; // no cardinality
    case Type::Property:
        json["propertySet"] = pset.toJson();
        json["baseName"] = property.toJson();
        json["value"] = value.toJson();
        if (!data_type.isEmpty()) json["dataType"] = data_type;
        break;
    case Type::Attribute:
        json["name"] = property.toJson();
        json["value"] = value.toJson();
        break;
    case Type::Classification:
        json["system"] = system.toJson();
        json["value"] = value.toJson();
        break;
    case Type::Material:
        json["value"] = value.toJson();
        break;
    case Type::PartOf:
        json["relation"] = relation;
        json["name"] = name.toJson();
        json["includeSubtypes"] = include_subtypes;
        json["predefinedType"] = predefined_type.toJson();
        break;
    }
    json["cardinality"] = cardinality;
    if (!uri.isEmpty()) json["uri"] = uri;
    if (!instructions.isEmpty()) json["instructions"] = instructions;
    return json;
}

IdsFacet IdsFacet::fromJson(const QJsonObject& json) {
    const QString key = json["type"].toString("entity");
    Type type = Type::Entity;
    for (Type candidate : {Type::Entity, Type::Property, Type::Attribute, Type::Classification, Type::Material,
                           Type::PartOf}) {
        if (key == typeKey(candidate)) type = candidate;
    }
    IdsFacet facet = ofType(type);
    facet.cardinality = json["cardinality"].toString("required");
    facet.uri = json["uri"].toString();
    facet.instructions = json["instructions"].toString();
    switch (type) {
    case Type::Entity:
        if (json["name"].isObject()) {
            facet.name = IdsValue::fromJson(json["name"]);
        } else if (json.contains("names")) { // worker 2.0
            IdsValue value;
            value.kind = IdsValue::Kind::Enumeration;
            for (const auto& name : json["names"].toArray()) value.values << name.toString();
            facet.name = value.values.size() == 1 ? IdsValue::simpleValue(value.values.front()) : value;
        } else {
            facet.name = IdsValue::simpleValue(json["name"].toString()); // worker 1.x
        }
        facet.include_subtypes = json["includeSubtypes"].toBool(false);
        facet.predefined_type = IdsValue::fromJson(json["predefinedType"]);
        facet.cardinality = "required";
        break;
    case Type::Property:
        facet.pset = IdsValue::fromJson(json["propertySet"]);
        facet.property = IdsValue::fromJson(json["baseName"]);
        facet.value = IdsValue::fromJson(json["value"]);
        facet.data_type = json["dataType"].toString();
        break;
    case Type::Attribute:
        facet.property = IdsValue::fromJson(json["name"]);
        facet.value = IdsValue::fromJson(json["value"]);
        break;
    case Type::Classification:
        facet.system = IdsValue::fromJson(json["system"]);
        facet.value = IdsValue::fromJson(json["value"]);
        break;
    case Type::Material:
        facet.value = IdsValue::fromJson(json["value"]);
        break;
    case Type::PartOf:
        facet.relation = json["relation"].toString();
        facet.name = IdsValue::fromJson(json["name"]);
        facet.include_subtypes = json["includeSubtypes"].toBool(false);
        facet.predefined_type = IdsValue::fromJson(json["predefinedType"]);
        break;
    }
    return facet;
}

// ---------------------------------------------------------------------------
// IdsBridge
// ---------------------------------------------------------------------------

namespace engine = ids_engine;

// A file the bridge opened itself (a path no loaded model provides).
struct IdsBridge::OwnFile {
    QString path;
    QDateTime modified;
    std::unique_ptr<ifcopenshell::file> file;
};

IdsBridge::IdsBridge(QObject* parent)
    : QObject(parent)
{
}

IdsBridge::~IdsBridge() = default;

ifcopenshell::file* IdsBridge::file(const QString& ifc_path, QString* error) {
    if (ifc_path.isEmpty()) {
        if (error) *error = "no model";
        return nullptr;
    }
    if (file_provider_) {
        if (auto* provided = file_provider_(ifc_path)) return provided;
    }
    const QFileInfo info(ifc_path);
    if (!info.isFile()) {
        if (error) *error = "the IFC file cannot be found: " + ifc_path;
        return nullptr;
    }
    for (auto it = own_files_.begin(); it != own_files_.end(); ++it) {
        if ((*it)->path != ifc_path) continue;
        if ((*it)->modified == info.lastModified()) return (*it)->file.get();
        own_files_.erase(it); // changed on disk: reopen
        break;
    }
    auto opened = std::make_unique<ifcopenshell::file>(ifc_path.toStdString());
    if (!opened->good()) {
        if (error) *error = "the IFC file cannot be read: " + ifc_path;
        return nullptr;
    }
    auto own = std::make_unique<OwnFile>();
    own->path = ifc_path;
    own->modified = info.lastModified();
    own->file = std::move(opened);
    own_files_.push_back(std::move(own));
    while (own_files_.size() > 4) own_files_.erase(own_files_.begin());
    return own_files_.back()->file.get();
}

void IdsBridge::post(std::function<void()> work) {
    QTimer::singleShot(0, this, [work = std::move(work)]() {
        try {
            work();
        } catch (...) {
            // Each request reports its own errors; nothing may escape the event loop.
        }
    });
}

namespace {

// Runs `body`; a UserError (or any engine failure) becomes the message
// `fail` receives.
template <typename Body, typename Fail>
void guarded(Body body, Fail fail) {
    try {
        body();
    } catch (const engine::UserError& e) {
        fail(QString::fromStdString(e.what()));
    } catch (const std::exception& e) {
        fail(QString("the IDS engine failed: %1").arg(e.what()));
    } catch (...) {
        fail(QStringLiteral("the IDS engine failed"));
    }
}

} // namespace

void IdsBridge::select(const QString& ifc_path,
                        const QString& ids_xml,
                        const std::optional<std::vector<int>>& limit_ids,
                        SelectionResult callback) {
    post([this, ifc_path, ids_xml, limit_ids, callback = std::move(callback)]() {
        guarded(
            [&]() {
                QString error;
                auto* ifc = file(ifc_path, &error);
                if (ifc == nullptr) return callback(false, {}, error);
                std::string xml_error;
                const auto document = engine::it::idsFromXml(ids_xml.toUtf8(), &xml_error);
                if (!document) return callback(false, {}, "the IDS document cannot be read: " + QString::fromStdString(xml_error));
                if (document->specifications.empty()) return callback(false, {}, "the IDS document has no specification");
                callback(true, engine::select(*ifc, document->specifications.front(), limit_ids), {});
            },
            [&](const QString& error) { callback(false, {}, error); });
    });
}

void IdsBridge::preview(const QString& ifc_path, const SelectionFields& fields, SelectionResult callback) {
    post([this, ifc_path, fields, callback = std::move(callback)]() {
        guarded(
            [&]() {
                QString error;
                auto* ifc = file(ifc_path, &error);
                if (ifc == nullptr) return callback(false, {}, "no model to preview on");
                const auto spec = engine::buildSpecification("preview", fields.facets, ifc);
                callback(true, engine::select(*ifc, spec), {});
            },
            [&](const QString& error) { callback(false, {}, error); });
    });
}

void IdsBridge::build(const QString& name, const SelectionFields& fields, XmlResult callback) {
    post([name, fields, callback = std::move(callback)]() {
        guarded(
            [&]() {
                const QString title = name.trimmed().isEmpty() ? QStringLiteral("Selection") : name.trimmed();
                callback(true, engine::buildXml(title, fields.facets), {});
            },
            [&](const QString& error) { callback(false, {}, error); });
    });
}

void IdsBridge::parse(const QString& ids_xml, FieldsResult callback) {
    post([ids_xml, callback = std::move(callback)]() {
        guarded(
            [&]() {
                SelectionFields fields;
                fields.facets = engine::parseXml(ids_xml);
                callback(true, fields, {});
            },
            [&](const QString& error) { callback(false, {}, error); });
    });
}

void IdsBridge::classes(const QString& ifc_path, ListResult callback) {
    post([this, ifc_path, callback = std::move(callback)]() {
        guarded(
            [&]() {
                QString error;
                auto* ifc = ifc_path.isEmpty() ? nullptr : file(ifc_path, &error);
                if (!ifc_path.isEmpty() && ifc == nullptr) return callback(false, {}, error);
                callback(true, engine::selectableClasses(ifc), {});
            },
            [&](const QString& error) { callback(false, {}, error); });
    });
}

void IdsBridge::classInfo(const QString& ifc_path, const IdsValue& name, bool include_subtypes,
                          ClassInfoResult callback) {
    post([this, ifc_path, name, include_subtypes, callback = std::move(callback)]() {
        guarded(
            [&]() {
                QString error;
                auto* ifc = ifc_path.isEmpty() ? nullptr : file(ifc_path, &error);
                if (!ifc_path.isEmpty() && ifc == nullptr) return callback(false, {}, error);
                callback(true, engine::classInfo(ifc, name, include_subtypes), {});
            },
            [&](const QString& error) { callback(false, {}, error); });
    });
}

void IdsBridge::properties(const QString& ifc_path, const IdsFacet& entity, PropertiesResult callback) {
    post([this, ifc_path, entity, callback = std::move(callback)]() {
        guarded(
            [&]() {
                QString error;
                auto* ifc = file(ifc_path, &error);
                if (ifc == nullptr) return callback(false, {}, {}, "no model to read the properties from");
                PropertyMap psets;
                SampleMap samples;
                engine::properties(*ifc, entity, psets, samples);
                callback(true, psets, samples, {});
            },
            [&](const QString& error) { callback(false, {}, {}, error); });
    });
}

void IdsBridge::values(const QString& ifc_path, const IdsFacet& entity, const QString& pset,
                       const QString& property, ValuesResult callback) {
    post([this, ifc_path, entity, pset, property, callback = std::move(callback)]() {
        guarded(
            [&]() {
                QString error;
                auto* ifc = file(ifc_path, &error);
                if (ifc == nullptr) return callback(false, {}, "no model to read the values from");
                callback(true, engine::values(*ifc, entity, pset, property), {});
            },
            [&](const QString& error) { callback(false, {}, error); });
    });
}

} // namespace bonsaiviewer::services
