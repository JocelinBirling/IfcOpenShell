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

#include "DocumentStore.h"

#include <QDir>
#include <QFileInfo>

namespace bonsaiviewer::modules::external_documents {

DocumentStore::DocumentStore(QObject* parent) : QObject(parent) {}

FolderNode* DocumentStore::folder(const QString& id) {
    for (FolderNode& f : folders_) if (f.id == id) return &f;
    return nullptr;
}

ExternalDocument* DocumentStore::document(const QString& id) {
    for (ExternalDocument& d : documents_) if (d.id == id) return &d;
    return nullptr;
}

QList<ExternalDocument*> DocumentStore::documentsInFolder(const QString& folder_id) const {
    QList<ExternalDocument*> out;
    for (const ExternalDocument& d : documents_) {
        if (d.folder_id == folder_id) out.append(const_cast<ExternalDocument*>(&d));
    }
    return out;
}

FolderNode& DocumentStore::addFolder(const QString& name, const QString& parent_id) {
    folders_.append(FolderNode{newId(), name, parent_id});
    emit structureChanged();
    return folders_.last();
}

ExternalDocument& DocumentStore::addDocument(const QString& path, const QString& folder_id) {
    ExternalDocument document;
    document.path = path;
    document.name = QFileInfo(path).fileName();
    document.folder_id = folder_id;
    document.kind = kindOfFile(path);
    documents_.append(document);
    emit structureChanged();
    return documents_.last();
}

void DocumentStore::removeFolder(const QString& id) {
    FolderNode* self = folder(id);
    const QString parent_id = self != nullptr ? self->parent_id : QString();
    // Children (folders and documents) move up to the removed folder's parent.
    for (FolderNode& f : folders_) if (f.parent_id == id) f.parent_id = parent_id;
    for (ExternalDocument& d : documents_) if (d.folder_id == id) d.folder_id = parent_id;
    folders_.removeIf([&id](const FolderNode& f) { return f.id == id; });
    emit structureChanged();
}

void DocumentStore::removeDocument(const QString& id) {
    documents_.removeIf([&id](const ExternalDocument& d) { return d.id == id; });
    emit structureChanged();
}

void DocumentStore::renameFolder(const QString& id, const QString& name) {
    if (FolderNode* f = folder(id)) {
        f->name = name;
        emit structureChanged();
    }
}

void DocumentStore::renameDocument(const QString& id, const QString& name) {
    if (ExternalDocument* d = document(id)) {
        d->name = name;
        emit documentChanged(id);
    }
}

void DocumentStore::moveDocument(const QString& id, const QString& folder_id) {
    if (ExternalDocument* d = document(id)) {
        d->folder_id = folder_id;
        emit structureChanged();
    }
}

void DocumentStore::moveFolder(const QString& id, const QString& parent_id) {
    if (FolderNode* f = folder(id)) {
        f->parent_id = parent_id;
        emit structureChanged();
    }
}

QJsonObject DocumentStore::toJson() const {
    return treeToJson(folders_, documents_);
}

void DocumentStore::fromJson(const QJsonObject& json) {
    treeFromJson(json, folders_, documents_);
    emit structureChanged();
}

void DocumentStore::clear() {
    folders_.clear();
    documents_.clear();
    emit structureChanged();
}

QString resolveAgainst(const QString& path, const QString& json_file_path) {
    if (path.isEmpty() || QDir::isAbsolutePath(path)) return path;
    return QDir(QFileInfo(json_file_path).absolutePath()).filePath(path);
}

} // namespace bonsaiviewer::modules::external_documents
