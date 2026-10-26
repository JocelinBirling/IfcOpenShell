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

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QSettings>

#include <utility>

namespace bonsaiviewer::services {

namespace {

constexpr auto kInterpreterKey = "filters/interpreter";
constexpr auto kScriptKey = "filters/script";
constexpr auto kDefaultInterpreter = "python3";

QString interpreterPath() {
    QSettings settings;
    const QString configured = settings.value(kInterpreterKey).toString();
    return configured.isEmpty() ? QString::fromUtf8(kDefaultInterpreter) : configured;
}

QString scriptPath() {
    QSettings settings;
    return settings.value(kScriptKey).toString();
}

QString errorOf(const QJsonObject& reply) {
    const QString error = reply["error"].toString();
    return error.isEmpty() ? QStringLiteral("the IDS engine failed") : error;
}

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
    IdsFacet facet;
    facet.type = Type::Property;
    return facet;
}

QString IdsFacet::summary() const {
    if (type == Type::Entity) {
        QString label;
        switch (name.kind) {
        case IdsValue::Kind::Simple: label = name.simple; break;
        case IdsValue::Kind::Enumeration: label = QString("{%1}").arg(name.values.join(", ")); break;
        case IdsValue::Kind::Pattern: label = QString("/%1/").arg(name.pattern); break;
        default: label = QStringLiteral("(no class)"); break;
        }
        if (include_subtypes) label += "+";
        if (!predefined_type.isAny()) {
            label += predefined_type.kind == IdsValue::Kind::Simple
                ? QString(" / %1").arg(predefined_type.simple)
                : QString(" / %1").arg(predefined_type.summary());
        }
        return label;
    }
    const auto name_of = [](const IdsValue& value) {
        switch (value.kind) {
        case IdsValue::Kind::Simple: return value.simple;
        case IdsValue::Kind::Pattern: return QString("/%1/").arg(value.pattern);
        case IdsValue::Kind::Enumeration: return QString("{%1}").arg(value.values.join("|"));
        case IdsValue::Kind::Length: return QString("(%1)").arg(value.summary());
        case IdsValue::Kind::Any:
        case IdsValue::Kind::Range: break;
        }
        return QStringLiteral("?");
    };
    QString label = QString("%1.%2").arg(name_of(pset), name_of(property));
    if (cardinality == "prohibited") return "no " + label;
    if (!value.isAny()) label += " " + value.summary();
    else label += " exists";
    if (cardinality == "optional") label += " (optional)";
    return label;
}

QJsonObject IdsFacet::toJson() const {
    QJsonObject json;
    if (type == Type::Entity) {
        json["type"] = "entity";
        json["name"] = name.toJson();
        json["includeSubtypes"] = include_subtypes;
        json["predefinedType"] = predefined_type.toJson();
    } else {
        json["type"] = "property";
        json["propertySet"] = pset.toJson();
        json["baseName"] = property.toJson();
        json["value"] = value.toJson();
        json["cardinality"] = cardinality;
    }
    return json;
}

IdsFacet IdsFacet::fromJson(const QJsonObject& json) {
    IdsFacet facet;
    if (json["type"].toString() == "property") {
        facet.type = Type::Property;
        facet.pset = IdsValue::fromJson(json["propertySet"]);
        facet.property = IdsValue::fromJson(json["baseName"]);
        facet.value = IdsValue::fromJson(json["value"]);
        facet.cardinality = json["cardinality"].toString("required");
    } else {
        facet.type = Type::Entity;
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
    }
    return facet;
}

// ---------------------------------------------------------------------------
// IdsBridge
// ---------------------------------------------------------------------------

IdsBridge::IdsBridge(QObject* parent)
    : QObject(parent)
{
}

bool IdsBridge::isConfigured() {
    if (scriptPath().isEmpty() || !QFileInfo::exists(scriptPath())) {
        return false;
    }
    QProcess probe;
    probe.start(interpreterPath(), {"-c", "import ifcopenshell, ifctester"});
    return probe.waitForFinished(10000) && probe.exitStatus() == QProcess::NormalExit &&
           probe.exitCode() == 0;
}

QJsonObject IdsBridge::entityJson(const IdsFacet& entity) {
    QJsonObject json;
    json["name"] = entity.name.toJson();
    json["includeSubtypes"] = entity.include_subtypes;
    return json;
}

QJsonArray IdsBridge::facetsJson(const SelectionFields& fields) {
    QJsonArray facets;
    for (const auto& facet : fields.facets) facets.append(facet.toJson());
    return facets;
}

void IdsBridge::selectionReply(const QJsonObject& reply, const SelectionResult& callback) {
    if (!reply["ok"].toBool()) {
        callback(false, {}, errorOf(reply));
        return;
    }
    std::vector<int> ids;
    const QJsonArray array = reply["ids"].toArray();
    ids.reserve(array.size());
    for (const auto& value : array) ids.push_back(value.toInt());
    callback(true, std::move(ids), {});
}

void IdsBridge::select(const QString& ifc_path,
                        const QString& ids_xml,
                        const std::optional<std::vector<int>>& limit_ids,
                        SelectionResult callback) {
    QJsonObject request;
    request["op"] = "select";
    request["ifc"] = ifc_path;
    request["ids_xml"] = ids_xml;
    if (limit_ids) {
        QJsonArray array;
        for (int id : *limit_ids) array.append(id);
        request["limit_ids"] = array;
    }
    send(request, [callback = std::move(callback)](const QJsonObject& reply) {
        selectionReply(reply, callback);
    });
}

void IdsBridge::preview(const QString& ifc_path, const SelectionFields& fields, SelectionResult callback) {
    QJsonObject request;
    request["op"] = "preview";
    request["ifc"] = ifc_path;
    request["facets"] = facetsJson(fields);
    send(request, [callback = std::move(callback)](const QJsonObject& reply) {
        selectionReply(reply, callback);
    });
}

void IdsBridge::build(const QString& name, const SelectionFields& fields, XmlResult callback) {
    QJsonObject request;
    request["op"] = "build";
    request["name"] = name;
    request["facets"] = facetsJson(fields);
    send(request, [callback = std::move(callback)](const QJsonObject& reply) {
        if (!reply["ok"].toBool()) {
            callback(false, {}, errorOf(reply));
            return;
        }
        callback(true, reply["ids_xml"].toString(), {});
    });
}

void IdsBridge::parse(const QString& ids_xml, FieldsResult callback) {
    QJsonObject request;
    request["op"] = "parse";
    request["ids_xml"] = ids_xml;
    send(request, [callback = std::move(callback)](const QJsonObject& reply) {
        SelectionFields fields;
        if (!reply["ok"].toBool()) {
            callback(false, fields, errorOf(reply));
            return;
        }
        for (const auto& value : reply["facets"].toArray()) {
            fields.facets.push_back(IdsFacet::fromJson(value.toObject()));
        }
        callback(true, fields, {});
    });
}

void IdsBridge::classes(const QString& ifc_path, ListResult callback) {
    QJsonObject request;
    request["op"] = "classes";
    if (!ifc_path.isEmpty()) request["ifc"] = ifc_path;
    send(request, [callback = std::move(callback)](const QJsonObject& reply) {
        if (!reply["ok"].toBool()) {
            callback(false, {}, errorOf(reply));
            return;
        }
        QStringList names;
        for (const auto& value : reply["classes"].toArray()) names << value.toString();
        callback(true, names, {});
    });
}

void IdsBridge::classInfo(const QString& ifc_path, const IdsValue& name, bool include_subtypes,
                          ClassInfoResult callback) {
    QJsonObject request;
    request["op"] = "classInfo";
    request["name"] = name.toJson();
    request["includeSubtypes"] = include_subtypes;
    if (!ifc_path.isEmpty()) request["ifc"] = ifc_path;
    send(request, [callback = std::move(callback)](const QJsonObject& reply) {
        ClassInfo info;
        if (!reply["ok"].toBool()) {
            callback(false, info, errorOf(reply));
            return;
        }
        for (const auto& value : reply["classes"].toArray()) info.classes << value.toString();
        for (const auto& value : reply["predefinedTypes"].toArray()) {
            info.predefined_types << value.toString();
        }
        info.count = reply["count"].toInt(-1);
        info.warning = reply["warning"].toString();
        callback(true, info, {});
    });
}

void IdsBridge::properties(const QString& ifc_path, const IdsFacet& entity, PropertiesResult callback) {
    QJsonObject request;
    request["op"] = "properties";
    request["ifc"] = ifc_path;
    request["entity"] = entityJson(entity);
    send(request, [callback = std::move(callback)](const QJsonObject& reply) {
        PropertyMap psets;
        SampleMap samples;
        if (!reply["ok"].toBool()) {
            callback(false, psets, samples, errorOf(reply));
            return;
        }
        const QJsonObject json = reply["psets"].toObject();
        for (auto pset = json.begin(); pset != json.end(); ++pset) {
            auto& properties = psets[pset.key()];
            const QJsonObject names = pset.value().toObject();
            for (auto name = names.begin(); name != names.end(); ++name) {
                properties.insert(name.key(), name.value().toInt());
            }
        }
        const QJsonObject sample_json = reply["samples"].toObject();
        for (auto pset = sample_json.begin(); pset != sample_json.end(); ++pset) {
            const QJsonObject names = pset.value().toObject();
            for (auto name = names.begin(); name != names.end(); ++name) {
                QStringList values;
                for (const auto& value : name.value().toArray()) values << value.toString();
                samples[pset.key()].insert(name.key(), values);
            }
        }
        callback(true, psets, samples, {});
    });
}

void IdsBridge::values(const QString& ifc_path, const IdsFacet& entity, const QString& pset,
                       const QString& property, ValuesResult callback) {
    QJsonObject request;
    request["op"] = "values";
    request["ifc"] = ifc_path;
    request["entity"] = entityJson(entity);
    request["propertySet"] = pset;
    request["baseName"] = property;
    send(request, [callback = std::move(callback)](const QJsonObject& reply) {
        ValueStats stats;
        if (!reply["ok"].toBool()) {
            callback(false, stats, errorOf(reply));
            return;
        }
        for (const auto& value : reply["values"].toArray()) {
            const QJsonObject entry = value.toObject();
            stats.values.append({entry["value"].toString(), entry["count"].toInt()});
        }
        stats.missing = reply["missing"].toInt();
        stats.base = reply["base"].toString("string");
        callback(true, stats, {});
    });
}

void IdsBridge::ensureStarted() {
    if (process_ != nullptr) {
        return;
    }
    process_ = new QProcess(this);
    // The worker's tracebacks go to the application's stderr: an unread
    // stderr pipe would eventually fill up and block the worker.
    process_->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    connect(process_, &QProcess::readyReadStandardOutput, this, &IdsBridge::readReply);
    connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return; // crashes also emit finished()
        failPending(QString("the IDS engine could not be started (%1)").arg(interpreterPath()));
        dropProcess();
    });
    connect(process_, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        failPending("the IDS engine exited unexpectedly (check that ifcopenshell is "
                    "installed for the configured Python interpreter)");
        dropProcess();
    });
    process_->start(interpreterPath(), {scriptPath()});
}

void IdsBridge::dropProcess() {
    if (process_ == nullptr) return;
    process_->disconnect(this);
    process_->deleteLater();
    process_ = nullptr;
    buffer_.clear();
}

void IdsBridge::send(const QJsonObject& request,
                     const std::function<void(const QJsonObject&)>& handler) {
    if (scriptPath().isEmpty() || !QFileInfo::exists(scriptPath())) {
        handler(QJsonObject{{"ok", false},
                            {"error", "the IDS engine is not configured: set the Python interpreter "
                                      "and the ifctester_ipc.py script in Settings > Filters"}});
        return;
    }
    ensureStarted();
    QJsonObject with_id = request;
    with_id["id"] = next_request_id_++;
    ++pending_requests_;
    handlers_.push_back({with_id["id"].toInt(), handler});
    // The worker is one-reply-per-request in FIFO order, so the write can
    // happen even while the process is still starting: the pipe buffers.
    const QByteArray line = QJsonDocument(with_id).toJson(QJsonDocument::Compact) + '\n';
    process_->write(line);
}

void IdsBridge::failPending(const QString& error) {
    // Swap first: a handler may send a new request (which re-registers a
    // handler) while we iterate.
    auto handlers = std::move(handlers_);
    handlers_.clear();
    pending_requests_ = 0;
    for (auto& [id, handler] : handlers) {
        handler(QJsonObject{{"ok", false}, {"error", error}});
    }
}

void IdsBridge::readReply() {
    buffer_.append(process_->readAllStandardOutput());
    int newline;
    while ((newline = buffer_.indexOf('\n')) >= 0) {
        const QByteArray line = buffer_.left(newline);
        buffer_.remove(0, newline + 1);
        if (line.trimmed().isEmpty()) continue;
        const QJsonObject reply = QJsonDocument::fromJson(line).object();
        if (reply.isEmpty()) continue;
        const int id = reply["id"].toInt(-1);
        for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
            if (it->first == id) {
                auto handler = std::move(it->second);
                handlers_.erase(it);
                --pending_requests_;
                handler(reply);
                break;
            }
        }
    }
}

} // namespace bonsaiviewer::services
