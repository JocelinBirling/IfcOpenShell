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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_TYPES_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_TYPES_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUuid>

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer35 / BonsaiViewer36 — the External Documents tree holds two
// kinds of documents, detected from the file extension: external tables
// (.xlsx/.csv, joined to the models) and 2D plans (.pdf in phase 1, then
// .png/.jpg/.svg/.dxf/.dwg, drawn as a background of the 3D view).

enum class DocumentKind { Table, Plan };

enum class DocumentState {
    Loaded,
    FileNotFound,
    KeyColumnMissing, // table only: the configured key column vanished from the file
};

QString kindToString(DocumentKind kind);
QString stateToString(DocumentState state);

// True when `path` is a document this module can import.
bool isSupportedDocument(const QString& path);
// The kind of a supported document, from its extension. Assumes
// isSupportedDocument(path).
DocumentKind kindOfFile(const QString& path);

QString newId();

// Join configuration of an external table (BonsaiViewer35).
struct JoinConfig {
    // Model key: "GlobalId" (default), "Name", "Class" or "Property".
    QString model_key = QStringLiteral("GlobalId");
    // Only when model_key == "Property" (BonsaiViewer26 property field).
    QString property_pset;
    QString property_name;
    // Column of the file holding the key; default: the first column.
    QString table_key_column;
    // .xlsx only: the sheet to read; default: the first sheet.
    QString sheet;

    QJsonObject toJson() const;
    static JoinConfig fromJson(const QJsonObject& json);
};

// Plan configuration of a 2D plan (BonsaiViewer36). Everything is saved with
// the project and exportable to / importable from a JSON file.
struct PlanConfig {
    double height_z = 0.0;                 // elevation on the model's Z axis, meters
    double rot_x = 0.0, rot_y = 0.0, rot_z = 0.0; // degrees
    double scale = 1.0;                    // manual factor on the plan's native size
    double trans_x = 0.0, trans_y = 0.0, trans_z = 0.0;
    bool trans_relative_to_plan = false;   // translation in plan axes instead of model axes
    int page = 0;                          // PDF only, 0-based; default: first page
    double opacity = 1.0;                  // 0..1, default opaque
    bool visible = true;

    QJsonObject toJson() const;
    static PlanConfig fromJson(const QJsonObject& json);
};

struct ExternalDocument {
    QString id = newId();
    QString name;        // named after its file
    QString path;        // absolute source path
    QString folder_id;   // empty = root of the tree
    DocumentKind kind = DocumentKind::Table;
    DocumentState state = DocumentState::Loaded;
    JoinConfig join;     // tables
    PlanConfig plan;     // plans

    QJsonObject toJson() const;
    static ExternalDocument fromJson(const QJsonObject& json);
};

struct FolderNode {
    QString id = newId();
    QString name;
    QString parent_id;   // empty = root

    QJsonObject toJson() const;
    static FolderNode fromJson(const QJsonObject& json);
};

// One .json file holds the configurations of all the documents of the tree
// (BonsaiViewer35/36: Save / Load Documents (JSON) on the root or a folder).
QJsonObject treeToJson(const QList<FolderNode>& folders, const QList<ExternalDocument>& documents);
void treeFromJson(const QJsonObject& json, QList<FolderNode>& folders, QList<ExternalDocument>& documents);

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_TYPES_H
