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

#include "Zip.h"

// QZipReader / QZipWriter moved from QtGui to QtCore (Qt 6.5+): use the
// header the Qt in use provides (CMake links CorePrivate, and GuiPrivate
// when it exists).
#if __has_include(<QtCore/private/qzipreader_p.h>)
#include <QtCore/private/qzipreader_p.h>
#include <QtCore/private/qzipwriter_p.h>
#else
#include <QtGui/private/qzipreader_p.h>
#include <QtGui/private/qzipwriter_p.h>
#endif

#include <QBuffer>
#include <QSaveFile>

namespace bonsaiviewer::ports::bcf::zip {

std::optional<std::map<QString, QByteArray>> read(const QString& path, QString* error) {
    QZipReader reader(path);
    if (!reader.isReadable() || reader.status() != QZipReader::NoError) {
        if (error) *error = QString("%1 is not a readable BCF (ZIP) file").arg(path);
        return std::nullopt;
    }
    std::map<QString, QByteArray> entries;
    for (const QZipReader::FileInfo& info : reader.fileInfoList()) {
        if (info.isDir || info.filePath.endsWith('/')) continue;
        entries[info.filePath] = reader.fileData(info.filePath);
    }
    reader.close();
    return entries;
}

bool write(const QString& path, const std::map<QString, QByteArray>& entries, QString* error) {
    // The archive is built in memory (QZipWriter closes its device), then
    // written through a temporary file: a failed save keeps the former file.
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    {
        QZipWriter writer(&buffer);
        writer.setCompressionPolicy(QZipWriter::AutoCompress);
        for (const auto& [name, data] : entries) writer.addFile(name, data);
        writer.close();
        if (writer.status() != QZipWriter::NoError) {
            if (error) *error = QString("Cannot build the archive of %1").arg(path);
            return false;
        }
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(buffer.data()) != buffer.data().size() || !file.commit()) {
        if (error) *error = QString("Cannot write %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

} // namespace bonsaiviewer::ports::bcf::zip
