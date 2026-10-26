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

#ifndef IFCINTERFACE_SERVICES_PROPERTYCATALOG_H
#define IFCINTERFACE_SERVICES_PROPERTYCATALOG_H

#include <QMap>
#include <QObject>
#include <QStringList>

#include <cstdint>
#include <vector>

class QTimer;

namespace bonsaiviewer { class SessionState; }

namespace bonsaiviewer::services {

class IfcDataAccess;

// B05 — PropertyCatalog (BonsaiViewer26, see 00_Technical_Implementation/
// BonsaiViewerTI07). The property sets and quantity sets found on the
// elements of all the loaded models, with element counts and a few sample
// values: the source of the standard property field. Built in the
// background, in small chunks on the GUI thread, the first time it is
// asked for and again after a model is added or removed.
class PropertyCatalog : public QObject {
    Q_OBJECT

public:
    // pset -> property -> number of elements carrying it.
    using PropertyMap = QMap<QString, QMap<QString, int>>;
    // pset -> property -> up to three distinct values.
    using SampleMap = QMap<QString, QMap<QString, QStringList>>;

    PropertyCatalog(SessionState& session, IfcDataAccess& data_access, QObject* parent = nullptr);

    // Starts a scan when the catalog is out of date; no-op otherwise.
    void ensureBuilt();
    bool isReady() const { return ready_; }
    bool isBuilding() const { return !pending_.empty(); }

    const PropertyMap& properties() const { return properties_; }
    const SampleMap& samples() const { return samples_; }
    // The property sets, the most used first.
    QStringList propertySets() const;

signals:
    // The scan is complete (or the catalog was emptied).
    void updated();

private:
    void invalidate();
    void scanChunk();

    SessionState& session_;
    IfcDataAccess& data_access_;
    QTimer* timer_ = nullptr;
    bool ready_ = false;
    bool stale_ = true;
    std::vector<std::uint32_t> pending_;
    PropertyMap properties_;
    SampleMap samples_;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_PROPERTYCATALOG_H
