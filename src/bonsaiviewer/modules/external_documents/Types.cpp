// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "Types.h"

#include <QFileInfo>
#include <QJsonArray>

namespace bonsaiviewer::modules::external_documents {

namespace {

constexpr int kJsonVersion = 1;

QString suffixOf(const QString& path) {
    return QFileInfo(path).suffix().toLower();
}

} // namespace

QString kindToString(DocumentKind kind) {
    return kind == DocumentKind::Table ? QStringLiteral("table") : QStringLiteral("plan");
}

QString stateToString(DocumentState state) {
    switch (state) {
    case DocumentState::Loaded: return QStringLiteral("loaded");
    case DocumentState::FileNotFound: return QStringLiteral("file not found");
    case DocumentState::KeyColumnMissing: return QStringLiteral("key column missing");
    }
    return {};
}

bool isSupportedDocument(const QString& path) {
    const QString suffix = suffixOf(path);
    return suffix == QLatin1String("csv") || suffix == QLatin1String("xlsx") ||
           suffix == QLatin1String("pdf") || suffix == QLatin1String("png") ||
           suffix == QLatin1String("jpg") || suffix == QLatin1String("jpeg") ||
           suffix == QLatin1String("svg") || suffix == QLatin1String("dxf") ||
           suffix == QLatin1String("dwg");
}

DocumentKind kindOfFile(const QString& path) {
    const QString suffix = suffixOf(path);
    if (suffix == QLatin1String("csv") || suffix == QLatin1String("xlsx")) return DocumentKind::Table;
    return DocumentKind::Plan;
}

QString newId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QJsonObject JoinConfig::toJson() const {
    QJsonObject json;
    json["model_key"] = model_key;
    json["property_pset"] = property_pset;
    json["property_name"] = property_name;
    json["table_key_column"] = table_key_column;
    json["sheet"] = sheet;
    return json;
}

JoinConfig JoinConfig::fromJson(const QJsonObject& json) {
    JoinConfig config;
    config.model_key = json["model_key"].toString(QStringLiteral("GlobalId"));
    config.property_pset = json["property_pset"].toString();
    config.property_name = json["property_name"].toString();
    config.table_key_column = json["table_key_column"].toString();
    config.sheet = json["sheet"].toString();
    return config;
}

QJsonObject PlanConfig::toJson() const {
    QJsonObject json;
    json["height_z"] = height_z;
    json["rot_x"] = rot_x;
    json["rot_y"] = rot_y;
    json["rot_z"] = rot_z;
    json["scale"] = scale;
    json["trans_x"] = trans_x;
    json["trans_y"] = trans_y;
    json["trans_z"] = trans_z;
    json["trans_relative_to_plan"] = trans_relative_to_plan;
    json["page"] = page;
    json["opacity"] = opacity;
    json["visible"] = visible;
    return json;
}

PlanConfig PlanConfig::fromJson(const QJsonObject& json) {
    PlanConfig config;
    config.height_z = json["height_z"].toDouble();
    config.rot_x = json["rot_x"].toDouble();
    config.rot_y = json["rot_y"].toDouble();
    config.rot_z = json["rot_z"].toDouble();
    config.scale = json["scale"].toDouble(1.0);
    config.trans_x = json["trans_x"].toDouble();
    config.trans_y = json["trans_y"].toDouble();
    config.trans_z = json["trans_z"].toDouble();
    config.trans_relative_to_plan = json["trans_relative_to_plan"].toBool();
    config.page = json["page"].toInt();
    config.opacity = json["opacity"].toDouble(1.0);
    config.visible = json["visible"].toBool(true);
    return config;
}

QJsonObject ExternalDocument::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["path"] = path;
    json["folder_id"] = folder_id;
    json["kind"] = kindToString(kind);
    json["state"] = stateToString(state);
    json["join"] = join.toJson();
    json["plan"] = plan.toJson();
    return json;
}

ExternalDocument ExternalDocument::fromJson(const QJsonObject& json) {
    ExternalDocument document;
    document.id = json["id"].toString(newId());
    document.name = json["name"].toString();
    document.path = json["path"].toString();
    document.folder_id = json["folder_id"].toString();
    document.kind = json["kind"].toString() == QLatin1String("plan") ? DocumentKind::Plan : DocumentKind::Table;
    const QString state = json["state"].toString();
    if (state == QLatin1String("file not found")) document.state = DocumentState::FileNotFound;
    else if (state == QLatin1String("key column missing")) document.state = DocumentState::KeyColumnMissing;
    else document.state = DocumentState::Loaded;
    document.join = JoinConfig::fromJson(json["join"].toObject());
    document.plan = PlanConfig::fromJson(json["plan"].toObject());
    return document;
}

QJsonObject FolderNode::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["parent_id"] = parent_id;
    return json;
}

FolderNode FolderNode::fromJson(const QJsonObject& json) {
    FolderNode folder;
    folder.id = json["id"].toString(newId());
    folder.name = json["name"].toString();
    folder.parent_id = json["parent_id"].toString();
    return folder;
}

QJsonObject treeToJson(const QList<FolderNode>& folders, const QList<ExternalDocument>& documents) {
    QJsonObject json;
    json["version"] = kJsonVersion;
    QJsonArray folder_array;
    for (const FolderNode& folder : folders) folder_array.append(folder.toJson());
    json["folders"] = folder_array;
    QJsonArray document_array;
    for (const ExternalDocument& document : documents) document_array.append(document.toJson());
    json["documents"] = document_array;
    return json;
}

void treeFromJson(const QJsonObject& json, QList<FolderNode>& folders, QList<ExternalDocument>& documents) {
    folders.clear();
    documents.clear();
    for (const QJsonValue& value : json["folders"].toArray()) {
        folders.append(FolderNode::fromJson(value.toObject()));
    }
    for (const QJsonValue& value : json["documents"].toArray()) {
        documents.append(ExternalDocument::fromJson(value.toObject()));
    }
}

} // namespace bonsaiviewer::modules::external_documents
