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

#ifndef BONSAIVIEWER_PORTS_BCF_ZIP_H
#define BONSAIVIEWER_PORTS_BCF_ZIP_H

#include <QByteArray>
#include <QString>

#include <map>
#include <optional>

namespace bonsaiviewer::ports::bcf::zip {

// The ZIP layer of the BCF archive (Python: zipfile / InMemoryZipFile),
// on Qt's private QZipReader / QZipWriter: the only file of the port that
// uses them. Folder entries are left out on reading.

// path -> bytes; nullopt when the file is not a readable ZIP.
std::optional<std::map<QString, QByteArray>> read(const QString& path, QString* error = nullptr);
// Writes the entries (deflated) to `path`, replacing it. False on failure.
bool write(const QString& path, const std::map<QString, QByteArray>& entries, QString* error = nullptr);

} // namespace bonsaiviewer::ports::bcf::zip

#endif // BONSAIVIEWER_PORTS_BCF_ZIP_H
