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
#include <memory>
#include <optional>
#include <vector>

namespace ifcopenshell { class file; }

namespace bonsaiviewer::services {

// One IDS field value: no constraint, a simple value, or one of the XSD
// restrictions the editor supports. Any IDS field that accepts a
// restriction (class, PredefinedType, property set, property, value...)
// uses this.
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

// One IDS facet of a selection. The six facets of IDS 1.0; each type only
// uses its own fields:
//   Entity          name (+ include_subtypes), predefined_type
//   Property        pset, property, value, data_type, cardinality
//   Attribute       property (the attribute name), value, cardinality
//   Classification  system, value, cardinality
//   Material        value, cardinality
//   PartOf          relation, name (+ include_subtypes), predefined_type, cardinality
struct IdsFacet {
    enum class Type {
        Entity,
        Property,
        Attribute,
        Classification,
        Material,
        PartOf,
    };
    Type type = Type::Entity;

    // Entity / PartOf: the class. IDS matches the class exactly;
    // include_subtypes widens it to the subtypes (IfcWall +
    // IfcWallStandardCase in IFC2x3).
    IdsValue name; // one class (simple), several (enumeration) or a pattern
    bool include_subtypes = true;
    IdsValue predefined_type;

    IdsValue pset;
    IdsValue property;
    IdsValue value;
    IdsValue system;
    QString data_type; // Property: IFC measure (IFCLABEL...), empty = any
    QString relation;  // PartOf: IFCRELAGGREGATES..., empty = any relation
    QString cardinality = "required"; // required / optional / prohibited
    // IDS metadata, kept for the IDS panel: the facet's URI (bSDD...) and
    // its instructions to the author of the model. Empty when unset.
    QString uri;
    QString instructions;

    static IdsFacet entity(const QString& ifc_class);
    static IdsFacet propertyFacet();
    static IdsFacet ofType(Type type);
    static QString typeLabel(Type type); // "Entity", "Property", "Part of"...
    QString summary() const;

    QJsonObject toJson() const;
    static IdsFacet fromJson(const QJsonObject& json);
};

// B06 filters / B24 IDS — the IDS engine bridge. All IDS work (selection,
// canonical serialisation, parsing, schema and model introspection) runs
// in-process on the C++ port of IfcTester (ports/ifctester, through
// IdsEngine), on the IFC files the viewer already has open. The requests
// keep their asynchronous form: every callback runs from the event loop,
// in request order.
class IdsBridge : public QObject {
    Q_OBJECT

public:
    // The facets of one selection: exactly one entity facet and any number
    // of other facets, all of which must pass.
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

    // The open IFC file of a model path (nullptr when not loaded).
    using FileProvider = std::function<ifcopenshell::file*(const QString& ifc_path)>;

    using SelectionResult = std::function<void(bool ok, std::vector<int> express_ids, const QString& error)>;
    using XmlResult = std::function<void(bool ok, const QString& ids_xml, const QString& error)>;
    using FieldsResult = std::function<void(bool ok, const SelectionFields& fields, const QString& error)>;
    using ListResult = std::function<void(bool ok, const QStringList& values, const QString& error)>;
    using ClassInfoResult = std::function<void(bool ok, const ClassInfo& info, const QString& error)>;
    using PropertiesResult = std::function<void(bool ok, const PropertyMap& psets, const SampleMap& samples,
                                                const QString& error)>;
    using ValuesResult = std::function<void(bool ok, const ValueStats& stats, const QString& error)>;

    explicit IdsBridge(QObject* parent = nullptr);
    ~IdsBridge() override;

    // Set by the shell: the files of the loaded models. A path the provider
    // does not know is opened by the bridge itself (kept open while
    // unchanged on disk).
    void setFileProvider(FileProvider provider) { file_provider_ = std::move(provider); }
    // The open file of `ifc_path` (provider, then the bridge's own cache).
    ifcopenshell::file* file(const QString& ifc_path, QString* error = nullptr);

    // The EXPRESS ids of the entities passing the specification. No limit
    // evaluates the whole model; otherwise the candidates are restricted
    // to the given EXPRESS ids (Run on the box) — an empty limit selects
    // nothing.
    void select(const QString& ifc_path,
                const QString& ids_xml,
                const std::optional<std::vector<int>>& limit_ids,
                SelectionResult callback);

    // Facets -> canonical IDS XML.
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
    // The same, on the elements of a selection (all its facets).
    void selectionProperties(const QString& ifc_path, const SelectionFields& fields, PropertiesResult callback);
    void values(const QString& ifc_path, const IdsFacet& entity, const QString& pset,
                const QString& property, ValuesResult callback);

private:
    // Runs `work` from the event loop, after the requests already queued.
    void post(std::function<void()> work);

    struct OwnFile;
    FileProvider file_provider_;
    std::vector<std::unique_ptr<OwnFile>> own_files_;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_IDSBRIDGE_H
