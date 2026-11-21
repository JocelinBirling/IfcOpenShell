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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_DOCUMENTSTORE_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_DOCUMENTSTORE_H

#include "Types.h"

#include <QJsonObject>
#include <QObject>

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer35/36 — the data behind the External Documents tree: folders
// and documents, with the JSON form used both for the project (.ifcfed
// extension) and for the Save / Load Documents (JSON) files. The file
// contents are never stored, only the path and the configurations.
class DocumentStore : public QObject {
    Q_OBJECT

public:
    explicit DocumentStore(QObject* parent = nullptr);

    const QList<FolderNode>& folders() const { return folders_; }
    const QList<ExternalDocument>& documents() const { return documents_; }
    QList<FolderNode>& folders() { return folders_; }
    QList<ExternalDocument>& documents() { return documents_; }

    FolderNode* folder(const QString& id);
    ExternalDocument* document(const QString& id);
    QList<ExternalDocument*> documentsInFolder(const QString& folder_id) const;

    FolderNode& addFolder(const QString& name, const QString& parent_id);
    ExternalDocument& addDocument(const QString& path, const QString& folder_id);
    void removeFolder(const QString& id);   // documents move to the parent folder
    void removeDocument(const QString& id);
    void renameFolder(const QString& id, const QString& name);
    void renameDocument(const QString& id, const QString& name);
    void moveDocument(const QString& id, const QString& folder_id);
    void moveFolder(const QString& id, const QString& parent_id);

    // The whole tree as one JSON object (federation extension / JSON file).
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    void clear();

signals:
    // The tree structure changed (folders or documents added/removed/moved).
    void structureChanged();
    // One document changed (state, join or plan configuration).
    void documentChanged(const QString& id);

private:
    QList<FolderNode> folders_;
    QList<ExternalDocument> documents_;
};

// Resolves a possibly relative `path` against the directory of the JSON
// file it comes from; absolute paths are kept as-is.
QString resolveAgainst(const QString& path, const QString& json_file_path);

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_DOCUMENTSTORE_H
