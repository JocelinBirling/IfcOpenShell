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

#include "ResultStore.h"

#include <QJsonArray>
#include <QJsonDocument>

#include <algorithm>

namespace bonsaiviewer::modules::rule::store {

namespace {

QJsonObject elementToJson(const ResultElement& element) {
    QJsonObject json{{"model_id", element.model_id}, {"global_id", element.global_id}};
    if (element.step_id != 0) json["step_id"] = element.step_id;
    return json;
}

ResultElement elementFromJson(const QJsonValue& value) {
    const QJsonObject json = value.toObject();
    return ResultElement{json.value("model_id").toString(), json.value("global_id").toString(),
                         json.value("step_id").toInt()};
}

QJsonArray elementsToJson(const std::vector<ResultElement>& elements) {
    QJsonArray out;
    for (const ResultElement& element : elements) out.append(elementToJson(element));
    return out;
}

std::vector<ResultElement> elementsFromJson(const QJsonValue& value) {
    std::vector<ResultElement> out;
    for (const QJsonValue& element : value.toArray()) out.push_back(elementFromJson(element));
    return out;
}

// One side of a pair as the panel shows it.
QJsonObject sideToJson(const QString& name, const QString& guid, const QString& cls, const QString& model, int step_id) {
    QJsonObject json;
    if (!name.isEmpty()) json["name"] = name;
    if (!guid.isEmpty()) json["global_id"] = guid;
    if (!cls.isEmpty()) json["ifc_type"] = cls;
    if (!model.isEmpty()) json["model"] = model;
    if (step_id != 0) json["step_id"] = step_id;
    return json;
}

void sideFromJson(const QJsonObject& json, QString& name, QString& guid, QString& cls, QString& model, int& step_id) {
    name = json.value("name").toString();
    guid = json.value("global_id").toString();
    cls = json.value("ifc_type").toString();
    model = json.value("model").toString();
    step_id = json.value("step_id").toInt();
}

// The engine's JSON (nlohmann) <-> Qt's.
QJsonObject toQt(const ifcclash::Json& json) {
    return QJsonDocument::fromJson(QByteArray::fromStdString(json.dump())).object();
}

ifcclash::Json fromQt(const QJsonObject& json) {
    return ifcclash::Json::parse(QJsonDocument(json).toJson(QJsonDocument::Compact).toStdString());
}

QJsonObject rowToJson(const ResultRow& row) {
    QJsonObject json{{"key", row.key},
                     {"node_id", row.node_id},
                     {"rule", row.rule_label},
                     {"rule_type", row.rule_type},
                     {"status", row.status},
                     {"review", reviewName(row.review)}};
    if (row.complex) json["complex"] = true;
    if (!row.group.isEmpty()) json["group"] = row.group;
    if (!row.manual_group.isEmpty()) json["manual_group"] = row.manual_group;
    if (!row.measure.isEmpty()) json["measure"] = row.measure;
    if (!row.criticity.isEmpty()) json["criticity"] = QJsonArray::fromStringList(row.criticity);
    if (!row.actor.isEmpty()) json["actor"] = QJsonArray::fromStringList(row.actor);
    json["source"] = sideToJson(row.source_name, row.source_guid, row.source_class, row.source_model_name, row.source_ifc_id);
    const QJsonObject target =
        sideToJson(row.target_name, row.target_guid, row.target_class, row.target_model_name, row.target_ifc_id);
    if (!target.isEmpty()) json["target"] = target;
    json["sources"] = elementsToJson(row.sources);
    if (!row.targets.empty()) json["targets"] = elementsToJson(row.targets);
    if (row.record) json["record"] = toQt(ifcclash::to_json(*row.record));
    if (!row.topic_guid.isEmpty()) json["topic_guid"] = row.topic_guid;
    return json;
}

ResultRow rowFromJson(const QJsonObject& json, QStringList* warnings) {
    ResultRow row;
    row.key = json.value("key").toString();
    row.node_id = json.value("node_id").toString();
    row.rule_label = json.value("rule").toString();
    row.rule_type = json.value("rule_type").toString();
    row.status = json.value("status").toBool(true);
    row.review = reviewFromName(json.value("review").toString());
    row.complex = json.value("complex").toBool(false);
    row.group = json.value("group").toString();
    row.manual_group = json.value("manual_group").toString();
    row.measure = json.value("measure").toString();
    for (const QJsonValue& v : json.value("criticity").toArray()) row.criticity << v.toString();
    for (const QJsonValue& v : json.value("actor").toArray()) row.actor << v.toString();
    sideFromJson(json.value("source").toObject(), row.source_name, row.source_guid, row.source_class,
                 row.source_model_name, row.source_ifc_id);
    sideFromJson(json.value("target").toObject(), row.target_name, row.target_guid, row.target_class,
                 row.target_model_name, row.target_ifc_id);
    row.sources = elementsFromJson(json.value("sources"));
    row.targets = elementsFromJson(json.value("targets"));
    row.topic_guid = json.value("topic_guid").toString();
    if (json.value("record").isObject()) {
        try {
            row.record = std::make_shared<const ifcclash::ResultRecord>(
                ifcclash::record_from_json(fromQt(json.value("record").toObject())));
        } catch (const std::exception& e) {
            if (warnings) *warnings << QString("Result %1: its details cannot be read (%2)").arg(row.key, e.what());
        }
    }
    return row;
}

} // namespace

QString reviewName(ReviewStatus status) {
    return status == ReviewStatus::Valid     ? QStringLiteral("Valid")
           : status == ReviewStatus::Invalid ? QStringLiteral("Invalid")
                                             : QStringLiteral("Non Determined");
}

ReviewStatus reviewFromName(const QString& name) {
    if (name == QLatin1String("Valid")) return ReviewStatus::Valid;
    if (name == QLatin1String("Invalid")) return ReviewStatus::Invalid;
    return ReviewStatus::NonDetermined;
}

QJsonObject toJson(const SavedClash& clash) {
    QJsonObject json{{"format", kFormat}};
    if (!clash.rule_file.isEmpty()) {
        QJsonObject rule_file{{"document", clash.rule_file}};
        if (!clash.rule_file_path.isEmpty()) rule_file["path"] = clash.rule_file_path;
        json["rule_file"] = rule_file;
    }
    QJsonArray runs;
    for (const SavedRun& run : clash.runs) {
        QJsonArray models;
        for (const SavedModel& model : run.models) {
            models.append(QJsonObject{{"model_id", model.model_id}, {"name", model.name}, {"path", model.path},
                                      {"mtime", double(model.mtime)}, {"size", double(model.size)}});
        }
        runs.append(QJsonObject{{"id", run.id}, {"started", run.started}, {"seconds", run.seconds}, {"models", models}});
    }
    json["runs"] = runs;
    QJsonArray nodes;
    for (const SavedNode& node : clash.nodes) {
        QJsonObject entry{{"node_id", node.node_id}, {"run_id", node.run_id}, {"label", node.label}};
        if (!node.type.isEmpty()) entry["type"] = node.type;
        if (!node.rule.isEmpty()) entry["rule"] = node.rule;
        if (!node.text.isEmpty()) entry["text"] = node.text;
        QJsonObject inputs;
        if (!node.sources.empty()) inputs["sources"] = elementsToJson(node.sources);
        if (!node.targets.empty()) inputs["targets"] = elementsToJson(node.targets);
        if (!node.not_checked.empty()) inputs["not_checked"] = elementsToJson(node.not_checked);
        if (!inputs.isEmpty()) entry["inputs"] = inputs;
        nodes.append(entry);
    }
    json["nodes"] = nodes;
    QJsonArray results;
    for (const ResultRow& row : clash.results) results.append(rowToJson(row));
    json["results"] = results;
    if (!clash.set_aside.empty()) {
        QJsonArray set_aside;
        for (const ResultRow& row : clash.set_aside) set_aside.append(rowToJson(row));
        json["set_aside"] = set_aside;
    }
    QJsonArray messages;
    for (const auto& [node, text] : clash.messages) messages.append(QJsonObject{{"node_id", node}, {"text", text}});
    if (!messages.isEmpty()) json["messages"] = messages;
    if (!clash.bcf_file.isEmpty()) json["bcf_file"] = clash.bcf_file;
    if (!clash.numbers.empty()) {
        QJsonObject numbers;
        for (const auto& [key, number] : clash.numbers) numbers[key] = number;
        json["result_numbers"] = numbers;
        json["next_result_number"] = clash.next_number;
    }
    return json;
}

SavedClash fromJson(const QJsonObject& json, QStringList* warnings) {
    SavedClash clash;
    if (json.value("format").toInt(kFormat) > kFormat && warnings) {
        *warnings << QString("The clash results were saved by a newer viewer (format %1); some data may be lost.")
                         .arg(json.value("format").toInt());
    }
    const QJsonObject rule_file = json.value("rule_file").toObject();
    clash.rule_file = rule_file.value("document").toObject();
    clash.rule_file_path = rule_file.value("path").toString();
    for (const QJsonValue& value : json.value("runs").toArray()) {
        const QJsonObject entry = value.toObject();
        SavedRun run;
        run.id = entry.value("id").toString();
        run.started = entry.value("started").toString();
        run.seconds = entry.value("seconds").toDouble();
        for (const QJsonValue& m : entry.value("models").toArray()) {
            const QJsonObject model = m.toObject();
            run.models.push_back(SavedModel{model.value("model_id").toString(), model.value("name").toString(),
                                            model.value("path").toString(), qint64(model.value("mtime").toDouble()),
                                            qint64(model.value("size").toDouble())});
        }
        clash.runs.push_back(std::move(run));
    }
    for (const QJsonValue& value : json.value("nodes").toArray()) {
        const QJsonObject entry = value.toObject();
        SavedNode node;
        node.node_id = entry.value("node_id").toString();
        node.run_id = entry.value("run_id").toString();
        node.label = entry.value("label").toString();
        node.type = entry.value("type").toString();
        node.rule = entry.value("rule").toObject();
        node.text = entry.value("text").toString();
        const QJsonObject inputs = entry.value("inputs").toObject();
        node.sources = elementsFromJson(inputs.value("sources"));
        node.targets = elementsFromJson(inputs.value("targets"));
        node.not_checked = elementsFromJson(inputs.value("not_checked"));
        clash.nodes.push_back(std::move(node));
    }
    for (const QJsonValue& value : json.value("results").toArray()) {
        clash.results.push_back(rowFromJson(value.toObject(), warnings));
    }
    // Optional (older projects have none): the set-aside list.
    for (const QJsonValue& value : json.value("set_aside").toArray()) {
        clash.set_aside.push_back(rowFromJson(value.toObject(), warnings));
    }
    for (const QJsonValue& value : json.value("messages").toArray()) {
        const QJsonObject entry = value.toObject();
        clash.messages.emplace_back(entry.value("node_id").toString(), entry.value("text").toString());
    }
    clash.bcf_file = json.value("bcf_file").toString();
    const QJsonObject numbers = json.value("result_numbers").toObject();
    for (auto it = numbers.begin(); it != numbers.end(); ++it) {
        const int number = it.value().toInt();
        if (number <= 0) continue;
        clash.numbers[it.key()] = number;
        clash.next_number = std::max(clash.next_number, number + 1);
    }
    clash.next_number = std::max(clash.next_number, json.value("next_result_number").toInt(1));
    return clash;
}

} // namespace bonsaiviewer::modules::rule::store
