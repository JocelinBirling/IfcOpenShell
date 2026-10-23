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

#ifndef IFCINTERFACE_SERVICES_PERSISTENCESERVICE_H
#define IFCINTERFACE_SERVICES_PERSISTENCESERVICE_H

#include <QJsonDocument>
#include <QString>

#include <optional>

// B14 — PersistenceService (see 00_Technical_Implementation/
// BonsaiViewerTI00). JSON documents that survive closing the session:
// filters (TI06), table configurations (TI07), layouts (TI21), default
// paths (TI22a), shortcuts (TI22b). Documents live under the application
// data location; export/import copy them to arbitrary files so users can
// share them. Singleton, mirroring ViewerSettings::instance().

namespace bonsaiviewer::services {

class PersistenceService {
public:
    static PersistenceService& instance();

    // Full path of the named document ("<app data>/BonsaiViewer/<name>").
    QString documentPath(const QString& name) const;

    bool saveDocument(const QString& name, const QJsonDocument& document);
    std::optional<QJsonDocument> loadDocument(const QString& name) const;

    // Share a document with someone else: read/write through a file path.
    std::optional<QJsonDocument> importDocument(const QString& file_path) const;
    bool exportDocument(const QString& name, const QString& file_path) const;

private:
    PersistenceService() = default;

    // The document root, created on first call.
    QString documentDir() const;
};

} // namespace bonsaiviewer::services

#endif
