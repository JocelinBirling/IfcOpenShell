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

#ifndef BONSAIVIEWER_PORTS_BCF_BCFXML_H
#define BONSAIVIEWER_PORTS_BCF_BCFXML_H

#include "Bcf.h"

#include <QByteArray>
#include <QString>

#include <map>
#include <optional>

namespace bonsaiviewer::ports::bcf {

// Port of bcf.v2.bcfxml / bcf.v3.bcfxml (BcfXml.load, BcfXml.save): a .bcf
// archive (ZIP) <-> BcfFile. The version is read from bcf.version; files
// of version 2.0 are read as 2.1.

// Reads a .bcf file. nullopt with `error` set when it cannot be read.
std::optional<BcfFile> load(const QString& path, QString* error = nullptr);
// Writes `file` as `version` ("2.1" or "3.0"). False with `error` set.
bool save(const BcfFile& file, const QString& path, const QString& version, QString* error = nullptr);

// The same on the archive entries (path -> bytes), without the ZIP layer.
std::optional<BcfFile> fromEntries(const std::map<QString, QByteArray>& entries, QString* error = nullptr);
std::map<QString, QByteArray> toEntries(const BcfFile& file, const QString& version);

// One file of the archive (exposed for the tests).
std::optional<VisualizationInfo> parseVisualizationInfo(const QByteArray& xml, QString* error = nullptr);
QByteArray writeVisualizationInfo(const VisualizationInfo& info, const QString& version);

} // namespace bonsaiviewer::ports::bcf

#endif // BONSAIVIEWER_PORTS_BCF_BCFXML_H
