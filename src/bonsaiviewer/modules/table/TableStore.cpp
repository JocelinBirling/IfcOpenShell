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

#include "TableStore.h"

#include "../../services/PersistenceService.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

#include <algorithm>

namespace bonsaiviewer::modules::table {

namespace {

constexpr auto kStoreDocument = "tables.json";
constexpr auto kTableFormat = "bonsaiviewer-table";
constexpr int kFormatVersion = 1;

QString kindToString(TableColumn::Kind kind) {
    switch (kind) {
    case TableColumn::Kind::Property: return "property";
    case TableColumn::Kind::Name: return "name";
    case TableColumn::Kind::Class: return "class";
    case TableColumn::Kind::GlobalId: return "globalid";
    case TableColumn::Kind::Model: return "model";
    case TableColumn::Kind::Count: return "count";
    case TableColumn::Kind::Color: return "color";
    }
    return "property";
}

TableColumn::Kind kindFromString(const QString& value) {
    if (value == "name") return TableColumn::Kind::Name;
    if (value == "class") return TableColumn::Kind::Class;
    if (value == "globalid") return TableColumn::Kind::GlobalId;
    if (value == "model") return TableColumn::Kind::Model;
    if (value == "count") return TableColumn::Kind::Count;
    if (value == "color") return TableColumn::Kind::Color;
    return TableColumn::Kind::Property;
}

QJsonObject columnToJson(const TableColumn& column) {
    QJsonObject json;
    json["kind"] = kindToString(column.kind);
    json["title"] = column.title;
    json["pset"] = column.pset;
    json["property"] = column.property;
    json["auto_numeric"] = column.auto_numeric;
    return json;
}

TableColumn columnFromJson(const QJsonObject& json) {
    TableColumn column;
    column.kind = kindFromString(json["kind"].toString());
    column.title = json["title"].toString();
    column.pset = json["pset"].toString();
    column.property = json["property"].toString();
    column.auto_numeric = json["auto_numeric"].toBool(true);
    return column;
}

} // namespace

TableStore::TableStore(QObject* parent)
    : QObject(parent)
{
    load();
}

std::vector<TableDocument> TableStore::tablesInFolder(const QString& folder_id) const {
    std::vector<TableDocument> result;
    for (const auto& document : tables_) {
        if (document.folder_id == folder_id) result.push_back(document);
    }
    return result;
}

std::optional<TableDocument> TableStore::table(const QString& id) const {
    for (const auto& document : tables_) {
        if (document.id == id) return document;
    }
    return std::nullopt;
}

std::optional<TableFolder> TableStore::folder(const QString& id) const {
    for (const auto& entry : folders_) {
        if (entry.id == id) return entry;
    }
    return std::nullopt;
}

bool TableStore::isFolder(const QString& id) const {
    return folder(id).has_value();
}

QString TableStore::addFolder(const QString& name, const QString& parent_id) {
    TableFolder entry;
    entry.id = QUuid::createUuid().toString();
    entry.name = name;
    entry.parent_id = parent_id;
    folders_.push_back(entry);
    save();
    emit changed();
    return entry.id;
}

void TableStore::renameFolder(const QString& id, const QString& name) {
    for (auto& entry : folders_) {
        if (entry.id == id) {
            entry.name = name;
            save();
            emit changed();
            return;
        }
    }
}

void TableStore::deleteFolder(const QString& id) {
    // Collect the subtree, then drop folders and tables in one pass.
    std::vector<QString> doomed{id};
    for (size_t i = 0; i < doomed.size(); ++i) {
        for (const auto& entry : folders_) {
            if (entry.parent_id == doomed[i]) doomed.push_back(entry.id);
        }
    }
    folders_.erase(std::remove_if(folders_.begin(), folders_.end(), [&doomed](const TableFolder& entry) {
        return std::find(doomed.begin(), doomed.end(), entry.id) != doomed.end();
    }), folders_.end());
    tables_.erase(std::remove_if(tables_.begin(), tables_.end(), [&doomed](const TableDocument& document) {
        return std::find(doomed.begin(), doomed.end(), document.folder_id) != doomed.end();
    }), tables_.end());
    save();
    emit changed();
}

QString TableStore::addTable(const TableDocument& document, const QString& folder_id) {
    TableDocument stored = document;
    stored.id = QUuid::createUuid().toString();
    stored.folder_id = folder_id;
    tables_.push_back(stored);
    save();
    emit changed();
    return stored.id;
}

void TableStore::updateTable(const TableDocument& document) {
    for (auto& entry : tables_) {
        if (entry.id == document.id) {
            entry.definition = document.definition;
            save();
            emit changed();
            return;
        }
    }
}

void TableStore::deleteTable(const QString& id) {
    tables_.erase(std::remove_if(tables_.begin(), tables_.end(), [&id](const TableDocument& document) {
        return document.id == id;
    }), tables_.end());
    save();
    emit changed();
}

void TableStore::moveNode(const QString& id, const QString& new_parent) {
    if (isFolder(id) && !new_parent.isEmpty() && folderIsInSubtree(new_parent, id)) {
        return; // moving a folder inside itself would orphan the subtree
    }
    if (isFolder(id)) {
        for (auto& entry : folders_) {
            if (entry.id == id && entry.parent_id != new_parent) {
                entry.parent_id = new_parent;
                save();
                emit changed();
                return;
            }
        }
    } else {
        for (auto& entry : tables_) {
            if (entry.id == id && entry.folder_id != new_parent) {
                entry.folder_id = new_parent;
                save();
                emit changed();
                return;
            }
        }
    }
}

bool TableStore::exportTable(const QString& id, const QString& file_path) const {
    const auto document = table(id);
    if (!document) return false;
    QJsonObject envelope = tableToJson(*document);
    envelope.remove("id");
    envelope.remove("folder");
    envelope["format"] = QString::fromLatin1(kTableFormat);
    envelope["version"] = kFormatVersion;
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(QJsonDocument(envelope).toJson(QJsonDocument::Indented));
    return true;
}

QString TableStore::importTable(const QString& file_path, const QString& folder_id, QString& error) {
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = "the file cannot be read";
        return {};
    }
    QJsonParseError parse_error;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !json.isObject()) {
        error = QString("not a JSON file (%1)").arg(parse_error.errorString());
        return {};
    }
    const QJsonObject root = json.object();
    if (!root.contains("columns")) {
        error = "not a table file";
        return {};
    }
    if (root["version"].toInt(kFormatVersion) > kFormatVersion) {
        error = "the file was written by a newer Bonsai Viewer";
        return {};
    }
    TableDocument document = tableFromJson(root);
    document.id = QUuid::createUuid().toString();
    document.folder_id = folder_id;
    tables_.push_back(document);
    save();
    emit changed();
    return document.id;
}

QJsonObject TableStore::tableToJson(const TableDocument& document) {
    QJsonObject json;
    json["id"] = document.id;
    json["folder"] = document.folder_id;
    json["name"] = document.definition.name;
    json["description"] = document.definition.description;
    json["on_box"] = document.definition.on_box;
    QJsonArray facets;
    for (const auto& facet : document.definition.selection.facets) {
        facets.append(facet.toJson());
    }
    json["facets"] = facets;
    QJsonArray columns;
    for (const auto& column : document.definition.columns) {
        columns.append(columnToJson(column));
    }
    json["columns"] = columns;
    return json;
}

TableDocument TableStore::tableFromJson(const QJsonObject& json) {
    TableDocument document;
    document.id = json["id"].toString();
    document.folder_id = json["folder"].toString();
    document.definition.name = json["name"].toString();
    document.definition.description = json["description"].toString();
    document.definition.on_box = json["on_box"].toBool(false);
    for (const auto& value : json["facets"].toArray()) {
        document.definition.selection.facets.push_back(
            bonsaiviewer::services::IdsFacet::fromJson(value.toObject()));
    }
    for (const auto& value : json["columns"].toArray()) {
        document.definition.columns.push_back(columnFromJson(value.toObject()));
    }
    return document;
}

void TableStore::load() {
    folders_.clear();
    tables_.clear();
    const auto loaded = services::PersistenceService::instance().loadDocument(QLatin1String(kStoreDocument));
    if (!loaded) return;
    const QJsonObject root = loaded->object();
    for (const auto& value : root["folders"].toArray()) {
        const QJsonObject entry = value.toObject();
        TableFolder folder;
        folder.id = entry["id"].toString();
        folder.name = entry["name"].toString();
        folder.parent_id = entry["parent"].toString();
        folders_.push_back(folder);
    }
    for (const auto& value : root["tables"].toArray()) {
        tables_.push_back(tableFromJson(value.toObject()));
    }
}

void TableStore::save() const {
    QJsonObject root;
    QJsonArray folders;
    for (const auto& entry : folders_) {
        QJsonObject json;
        json["id"] = entry.id;
        json["name"] = entry.name;
        json["parent"] = entry.parent_id;
        folders.append(json);
    }
    root["folders"] = folders;
    QJsonArray tables;
    for (const auto& document : tables_) {
        tables.append(tableToJson(document));
    }
    root["tables"] = tables;
    services::PersistenceService::instance().saveDocument(QLatin1String(kStoreDocument),
                                                          QJsonDocument(root));
}

bool TableStore::folderIsInSubtree(const QString& candidate_id, const QString& root_id) const {
    QString current = candidate_id;
    while (!current.isEmpty()) {
        if (current == root_id) return true;
        const auto entry = folder(current);
        if (!entry) return false;
        current = entry->parent_id;
    }
    return false;
}

} // namespace bonsaiviewer::modules::table
