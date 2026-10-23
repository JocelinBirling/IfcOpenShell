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

#include "PersistenceService.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace bonsaiviewer::services {

PersistenceService& PersistenceService::instance() {
    static PersistenceService service;
    return service;
}

QString PersistenceService::documentDir() const {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                             + QStringLiteral("/BonsaiViewer");
    QDir().mkpath(root);
    return root;
}

QString PersistenceService::documentPath(const QString& name) const {
    return documentDir() + QStringLiteral("/") + name;
}

bool PersistenceService::saveDocument(const QString& name, const QJsonDocument& document) {
    QFile file(documentPath(name));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    return file.write(document.toJson()) == document.toJson().size();
}

std::optional<QJsonDocument> PersistenceService::loadDocument(const QString& name) const {
    QFile file(documentPath(name));
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    return QJsonDocument::fromJson(file.readAll());
}

std::optional<QJsonDocument> PersistenceService::importDocument(const QString& file_path) const {
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    return QJsonDocument::fromJson(file.readAll());
}

bool PersistenceService::exportDocument(const QString& name, const QString& file_path) const {
    const auto document = loadDocument(name);
    if (!document) {
        return false;
    }
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    return file.write(document->toJson()) == document->toJson().size();
}

} // namespace bonsaiviewer::services
