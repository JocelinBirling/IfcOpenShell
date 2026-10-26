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

#include "FilterStore.h"

#include "PersistenceService.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include <algorithm>

namespace bonsaiviewer::services {

namespace {

constexpr auto kStoreDocument = "filters.json";
constexpr auto kFilterFormat = "bonsaiviewer-filter";
constexpr auto kLibraryFormat = "bonsaiviewer-filter-library";
constexpr int kFormatVersion = 1;

FilterAction::Type actionTypeFromString(const QString& value) {
    static const QList<QPair<QString, FilterAction::Type>> table = {
        {"isolate", FilterAction::Type::Isolate},
        {"add", FilterAction::Type::AddToView},
        {"remove", FilterAction::Type::RemoveFromView},
        {"colour", FilterAction::Type::Colour},
        {"transparency", FilterAction::Type::Transparency},
        {"autocolour", FilterAction::Type::AutoColourByProperty},
    };
    for (const auto& [name, type] : table) {
        if (value == name) return type;
    }
    return FilterAction::Type::Isolate;
}

QString actionTypeToString(FilterAction::Type type) {
    switch (type) {
    case FilterAction::Type::Isolate: return "isolate";
    case FilterAction::Type::AddToView: return "add";
    case FilterAction::Type::RemoveFromView: return "remove";
    case FilterAction::Type::Colour: return "colour";
    case FilterAction::Type::Transparency: return "transparency";
    case FilterAction::Type::AutoColourByProperty: return "autocolour";
    }
    return "isolate";
}

} // namespace

FilterStore::FilterStore(QObject* parent)
    : QObject(parent)
{
    load();
}

std::vector<FilterDocument> FilterStore::filtersInFolder(const QString& folder_id) const {
    std::vector<FilterDocument> result;
    for (const auto& document : filters_) {
        if (document.folder_id == folder_id) result.push_back(document);
    }
    return result;
}

std::optional<FilterDocument> FilterStore::filter(const QString& id) const {
    for (const auto& document : filters_) {
        if (document.id == id) return document;
    }
    return std::nullopt;
}

std::optional<FilterFolder> FilterStore::folder(const QString& id) const {
    for (const auto& entry : folders_) {
        if (entry.id == id) return entry;
    }
    return std::nullopt;
}

bool FilterStore::isFolder(const QString& id) const {
    return folder(id).has_value();
}

bool FilterStore::folderIsEmpty(const QString& id) const {
    for (const auto& entry : folders_) {
        if (entry.parent_id == id) return false;
    }
    for (const auto& document : filters_) {
        if (document.folder_id == id) return false;
    }
    return true;
}

QString FilterStore::addFolder(const QString& name, const QString& parent_id) {
    FilterFolder entry;
    entry.id = QUuid::createUuid().toString();
    entry.name = name;
    entry.parent_id = parent_id;
    folders_.push_back(entry);
    save();
    emit changed();
    return entry.id;
}

void FilterStore::renameFolder(const QString& id, const QString& name) {
    for (auto& entry : folders_) {
        if (entry.id == id) {
            entry.name = name;
            save();
            emit changed();
            return;
        }
    }
}

void FilterStore::deleteFolder(const QString& id) {
    // Collect the subtree, then drop folders and filters in one pass.
    std::vector<QString> doomed{id};
    for (size_t i = 0; i < doomed.size(); ++i) {
        for (const auto& entry : folders_) {
            if (entry.parent_id == doomed[i]) doomed.push_back(entry.id);
        }
    }
    folders_.erase(std::remove_if(folders_.begin(), folders_.end(), [&doomed](const FilterFolder& entry) {
        return std::find(doomed.begin(), doomed.end(), entry.id) != doomed.end();
    }), folders_.end());
    filters_.erase(std::remove_if(filters_.begin(), filters_.end(), [&doomed](const FilterDocument& document) {
        return std::find(doomed.begin(), doomed.end(), document.folder_id) != doomed.end();
    }), filters_.end());
    save();
    emit changed();
}

QString FilterStore::addFilter(const FilterDocument& document, const QString& folder_id) {
    FilterDocument stored = document;
    stored.id = document.id.isEmpty() ? QUuid::createUuid().toString() : document.id;
    stored.folder_id = folder_id;
    filters_.push_back(stored);
    save();
    emit changed();
    return stored.id;
}

void FilterStore::updateFilter(const FilterDocument& document) {
    for (auto& entry : filters_) {
        if (entry.id == document.id) {
            entry.name = document.name;
            entry.description = document.description;
            entry.layers = document.layers;
            save();
            emit changed();
            return;
        }
    }
}

void FilterStore::deleteFilter(const QString& id) {
    filters_.erase(std::remove_if(filters_.begin(), filters_.end(), [&id](const FilterDocument& document) {
        return document.id == id;
    }), filters_.end());
    save();
    emit changed();
}

void FilterStore::moveNode(const QString& id, const QString& new_parent) {
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
        for (auto& entry : filters_) {
            if (entry.id == id && entry.folder_id != new_parent) {
                entry.folder_id = new_parent;
                save();
                emit changed();
                return;
            }
        }
    }
}

bool FilterStore::exportFilter(const QString& filter_id, const QString& file_path) const {
    const auto document = filter(filter_id);
    if (!document) return false;
    QJsonObject envelope = documentToJson(*document);
    envelope.remove("id");
    envelope.remove("folder");
    envelope["format"] = QString::fromLatin1(kFilterFormat);
    envelope["version"] = kFormatVersion;
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(QJsonDocument(envelope).toJson(QJsonDocument::Indented));
    return true;
}

QString FilterStore::importFilter(const QString& file_path, QString& error) {
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = "the file cannot be read";
        return {};
    }
    const QJsonObject envelope = QJsonDocument::fromJson(file.readAll()).object();
    if (envelope.isEmpty() || !envelope.contains("layers")) {
        error = "not a filter file";
        return {};
    }
    FilterDocument document = documentFromJson(envelope);
    document.id.clear();
    return addFilter(document, {});
}

bool FilterStore::exportLibrary(const QString& file_path) const {
    QJsonObject root;
    root["format"] = QString::fromLatin1(kLibraryFormat);
    root["version"] = kFormatVersion;
    QJsonArray folders;
    for (const auto& entry : folders_) {
        QJsonObject json;
        json["id"] = entry.id;
        json["name"] = entry.name;
        json["parent"] = entry.parent_id;
        folders.append(json);
    }
    root["folders"] = folders;
    QJsonArray filters;
    for (const auto& document : filters_) {
        filters.append(documentToJson(document));
    }
    root["filters"] = filters;
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

int FilterStore::importFile(const QString& file_path, const QString& folder_id, QString& error) {
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = "the file cannot be read";
        return -1;
    }
    QJsonParseError parse_error;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !json.isObject()) {
        error = QString("not a JSON file (%1)").arg(parse_error.errorString());
        return -1;
    }
    const QJsonObject root = json.object();
    if (root["version"].toInt(kFormatVersion) > kFormatVersion) {
        error = "the file was written by a newer Bonsai Viewer";
        return -1;
    }

    // A single filter envelope.
    if (root.contains("layers")) {
        FilterDocument document = documentFromJson(root);
        document.id = QUuid::createUuid().toString();
        document.folder_id = folder_id;
        filters_.push_back(document);
        save();
        emit changed();
        return 1;
    }

    // A library: folders first (parents may come after their children in
    // the file, so map every id before wiring the parents).
    if (!root.contains("filters")) {
        error = "not a filter file";
        return -1;
    }
    QHash<QString, QString> new_folder_id;
    for (const auto& value : root["folders"].toArray()) {
        new_folder_id.insert(value.toObject()["id"].toString(), QUuid::createUuid().toString());
    }
    for (const auto& value : root["folders"].toArray()) {
        const QJsonObject entry = value.toObject();
        FilterFolder imported;
        imported.id = new_folder_id.value(entry["id"].toString());
        imported.name = entry["name"].toString();
        imported.parent_id = new_folder_id.value(entry["parent"].toString(), folder_id);
        folders_.push_back(imported);
    }
    int count = 0;
    for (const auto& value : root["filters"].toArray()) {
        FilterDocument document = documentFromJson(value.toObject());
        document.id = QUuid::createUuid().toString();
        document.folder_id = new_folder_id.value(document.folder_id, folder_id);
        filters_.push_back(document);
        ++count;
    }
    save();
    emit changed();
    return count;
}

FilterDocument FilterStore::documentFromJson(const QJsonObject& json) {
    FilterDocument document;
    document.id = json["id"].toString();
    document.name = json["name"].toString();
    document.description = json["description"].toString();
    document.folder_id = json["folder"].toString();
    document.layers.reserve(json["layers"].toArray().size());
    for (const auto& value : json["layers"].toArray()) {
        document.layers.push_back(layerFromJson(value.toObject()));
    }
    return document;
}

QJsonObject FilterStore::documentToJson(const FilterDocument& document) {
    QJsonObject json;
    json["id"] = document.id;
    json["name"] = document.name;
    json["description"] = document.description;
    json["folder"] = document.folder_id;
    QJsonArray layers;
    for (const auto& layer : document.layers) {
        layers.append(layerToJson(layer));
    }
    json["layers"] = layers;
    return json;
}

FilterLayer FilterStore::layerFromJson(const QJsonObject& json) {
    FilterLayer layer;
    layer.is_and = json["and"].toBool();
    layer.ids_xml = json["ids"].toString();
    for (const auto& value : json["actions"].toArray()) {
        layer.actions.push_back(actionFromJson(value.toObject()));
    }
    return layer;
}

QJsonObject FilterStore::layerToJson(const FilterLayer& layer) {
    QJsonObject json;
    json["and"] = layer.is_and;
    json["ids"] = layer.ids_xml;
    QJsonArray actions;
    for (const auto& action : layer.actions) {
        actions.append(actionToJson(action));
    }
    json["actions"] = actions;
    return json;
}

FilterAction FilterStore::actionFromJson(const QJsonObject& json) {
    FilterAction action;
    action.type = actionTypeFromString(json["type"].toString());
    action.colour = QColor(json["color"].toString());
    action.transparency_percent = json["percent"].toInt();
    action.pset = json["pset"].toString();
    action.property = json["property"].toString();
    return action;
}

QJsonObject FilterStore::actionToJson(const FilterAction& action) {
    QJsonObject json;
    json["type"] = actionTypeToString(action.type);
    switch (action.type) {
    case FilterAction::Type::Colour:
        json["color"] = action.colour.name();
        break;
    case FilterAction::Type::Transparency:
        json["percent"] = action.transparency_percent;
        break;
    case FilterAction::Type::AutoColourByProperty:
        json["pset"] = action.pset;
        json["property"] = action.property;
        break;
    case FilterAction::Type::Isolate:
    case FilterAction::Type::AddToView:
    case FilterAction::Type::RemoveFromView:
        break;
    }
    return json;
}

void FilterStore::load() {
    folders_.clear();
    filters_.clear();
    const auto loaded = PersistenceService::instance().loadDocument(QLatin1String(kStoreDocument));
    if (!loaded) return;
    const QJsonObject root = loaded->object();
    for (const auto& value : root["folders"].toArray()) {
        const QJsonObject entry = value.toObject();
        FilterFolder folder;
        folder.id = entry["id"].toString();
        folder.name = entry["name"].toString();
        folder.parent_id = entry["parent"].toString();
        folders_.push_back(folder);
    }
    for (const auto& value : root["filters"].toArray()) {
        filters_.push_back(documentFromJson(value.toObject()));
    }
}

void FilterStore::save() const {
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
    QJsonArray filters;
    for (const auto& document : filters_) {
        filters.append(documentToJson(document));
    }
    root["filters"] = filters;
    PersistenceService::instance().saveDocument(QLatin1String(kStoreDocument),
                                                QJsonDocument(root));
}

bool FilterStore::folderIsInSubtree(const QString& candidate_id, const QString& root_id) const {
    QString current = candidate_id;
    while (!current.isEmpty()) {
        if (current == root_id) return true;
        const auto entry = folder(current);
        if (!entry) return false;
        current = entry->parent_id;
    }
    return false;
}

} // namespace bonsaiviewer::services
