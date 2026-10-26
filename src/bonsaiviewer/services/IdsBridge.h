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

#ifndef IFCINTERFACE_SERVICES_IDSBRIDGE_H
#define IFCINTERFACE_SERVICES_IDSBRIDGE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>

#include <functional>
#include <optional>
#include <vector>

class QProcess;

namespace bonsaiviewer::services {

// One IDS field value: no constraint, a simple value, or one of the XSD
// restrictions the editor supports. Any IDS field that accepts a
// restriction (class, PredefinedType, property set, property, value) uses
// this. Mirrors the worker's VALUE payload.
struct IdsValue {
    enum class Kind {
        Any,
        Simple,
        Enumeration,
        Pattern,
        Range,
        Length,
    };
    Kind kind = Kind::Any;
    QString simple;      // Simple
    QStringList values;  // Enumeration
    QString pattern;     // Pattern (XSD regular expression)
    QString min;         // Range / Length (characters); empty = unbounded
    QString max;
    bool min_inclusive = true;
    bool max_inclusive = true;
    QString base = "double"; // Range: double / integer

    static IdsValue simpleValue(const QString& text);
    bool isAny() const { return kind == Kind::Any; }
    // A short human-readable form ("= REI60", "∈ {A, B}", "≥ 0.2"...).
    QString summary() const;

    QJsonValue toJson() const;
    static IdsValue fromJson(const QJsonValue& json);
};

// One IDS facet of a selection. v1 supports the entity and the property
// facets; the other IDS facets (attribute, classification, material,
// partOf) slot in as new types with their own editor page.
struct IdsFacet {
    enum class Type {
        Entity,
        Property,
    };
    Type type = Type::Entity;

    // Entity facet. IDS matches the class exactly; include_subtypes widens
    // it to the subtypes (IfcWall + IfcWallStandardCase in IFC2x3).
    IdsValue name; // the class: one (simple), several (enumeration) or a pattern
    bool include_subtypes = true;
    IdsValue predefined_type;

    // Property facet.
    IdsValue pset;
    IdsValue property;
    IdsValue value;
    QString cardinality = "required"; // required / optional / prohibited

    static IdsFacet entity(const QString& ifc_class);
    static IdsFacet propertyFacet();
    QString summary() const;

    QJsonObject toJson() const;
    static IdsFacet fromJson(const QJsonObject& json);
};

// B06 filters — the IDS engine bridge. All IDS work (selection, canonical
// serialisation, parsing, schema and model introspection) is delegated to
// IfcTester through a long-lived JSON Lines worker process
// (src/ifctester/ifctester_ipc.py, same protocol pattern as the
// IfcClash_Plus ipc). The interpreter and script paths come from the
// settings keys "filters/interpreter" and "filters/script"; when they are
// not configured every request fails cleanly and the UI shows a popup
// instead of a half-working feature.
class IdsBridge : public QObject {
    Q_OBJECT

public:
    // The facets of one selection layer, as edited by the creation window:
    // exactly one entity facet and any number of property facets, all of
    // which must pass.
    struct SelectionFields {
        std::vector<IdsFacet> facets;
    };

    struct ClassInfo {
        QStringList classes;          // the classes the entity facet covers
        QStringList predefined_types; // the PredefinedType enumeration
        int count = -1;               // matching elements; -1 without a model
        QString warning;              // e.g. a class pattern matching no class
    };

    struct ValueStats {
        QList<QPair<QString, int>> values; // distinct value -> element count
        int missing = 0;                   // elements without the property
        QString base;                      // double / integer / boolean / string
    };

    // pset -> property -> number of elements carrying it.
    using PropertyMap = QMap<QString, QMap<QString, int>>;
    // pset -> property -> up to three of its values, for the picker.
    using SampleMap = QMap<QString, QMap<QString, QStringList>>;

    using SelectionResult = std::function<void(bool ok, std::vector<int> express_ids, const QString& error)>;
    using XmlResult = std::function<void(bool ok, const QString& ids_xml, const QString& error)>;
    using FieldsResult = std::function<void(bool ok, const SelectionFields& fields, const QString& error)>;
    using ListResult = std::function<void(bool ok, const QStringList& values, const QString& error)>;
    using ClassInfoResult = std::function<void(bool ok, const ClassInfo& info, const QString& error)>;
    using PropertiesResult = std::function<void(bool ok, const PropertyMap& psets, const SampleMap& samples,
                                                const QString& error)>;
    using ValuesResult = std::function<void(bool ok, const ValueStats& stats, const QString& error)>;

    explicit IdsBridge(QObject* parent = nullptr);

    // True when the script exists and the interpreter imports
    // ifcopenshell + ifctester. Blocks up to 10 s: settings only.
    static bool isConfigured();

    // The EXPRESS ids of the entities passing the specification. No limit
    // evaluates the whole model; otherwise the candidates are restricted
    // to the given EXPRESS ids (Run on the box) — an empty limit selects
    // nothing.
    void select(const QString& ifc_path,
                const QString& ids_xml,
                const std::optional<std::vector<int>>& limit_ids,
                SelectionResult callback);

    // Facets -> canonical IDS XML, built by IfcTester itself.
    void build(const QString& name, const SelectionFields& fields, XmlResult callback);

    // IDS XML -> facets, for reopening a layer in the creation window.
    void parse(const QString& ids_xml, FieldsResult callback);

    // Build + select in one request, for the live preview.
    void preview(const QString& ifc_path, const SelectionFields& fields, SelectionResult callback);

    // Introspection for the editor. `ifc_path` may be empty: the schema
    // lists then cover IFC2X3 + IFC4 + IFC4X3 and nothing is counted.
    void classes(const QString& ifc_path, ListResult callback);
    void classInfo(const QString& ifc_path, const IdsValue& name, bool include_subtypes,
                   ClassInfoResult callback);
    void properties(const QString& ifc_path, const IdsFacet& entity, PropertiesResult callback);
    void values(const QString& ifc_path, const IdsFacet& entity, const QString& pset,
                const QString& property, ValuesResult callback);

private:
    static QJsonObject entityJson(const IdsFacet& entity);
    static QJsonArray facetsJson(const SelectionFields& fields);
    static void selectionReply(const QJsonObject& reply, const SelectionResult& callback);

    void ensureStarted();
    void dropProcess();
    void send(const QJsonObject& request,
              const std::function<void(const QJsonObject&)>& handler);
    void failPending(const QString& error);
    void readReply();

    QProcess* process_ = nullptr;
    QByteArray buffer_;
    int next_request_id_ = 1;
    int pending_requests_ = 0;
    // The completion handlers of the requests in flight, by request id.
    std::vector<std::pair<int, std::function<void(const QJsonObject&)>>> handlers_;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_IDSBRIDGE_H
