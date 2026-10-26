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

#include "PropertyCatalog.h"

#include "IfcDataAccess.h"
#include "../ElementRegistry.h"
#include "../SessionState.h"

#include <QTimer>

#include <algorithm>
#include <sstream>

namespace bonsaiviewer::services {

namespace {

constexpr int kChunkSize = 250;
constexpr int kMaxSamples = 3;

// Displayable scalar values only: lists, maps and entities are skipped.
QString sampleText(const property_value& value) {
    if (value.is_null() || value.get_if<express::base>() != nullptr
        || value.get_if<property_list>() != nullptr || value.get_if<property_map>() != nullptr) {
        return {};
    }
    std::ostringstream text;
    text << value;
    return QString::fromStdString(text.str());
}

} // namespace

PropertyCatalog::PropertyCatalog(SessionState& session, IfcDataAccess& data_access, QObject* parent)
    : QObject(parent), session_(session), data_access_(data_access), timer_(new QTimer(this))
{
    timer_->setInterval(0);
    connect(timer_, &QTimer::timeout, this, &PropertyCatalog::scanChunk);
    connect(&session_, &SessionState::modelsChanged, this, &PropertyCatalog::invalidate);
    connect(&session_, &SessionState::modelDataSourceReady, this, &PropertyCatalog::invalidate);
}

void PropertyCatalog::invalidate() {
    stale_ = true;
    // A scan in progress restarts so it covers the new set of models.
    if (isBuilding()) {
        timer_->stop();
        pending_.clear();
        ensureBuilt();
    }
}

void PropertyCatalog::ensureBuilt() {
    if (!stale_ || isBuilding()) return;
    stale_ = false;
    ready_ = false;
    properties_.clear();
    samples_.clear();
    for (uint32_t model_id : session_.sessionModelIds()) {
        for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(model_id)) {
            pending_.push_back(info.object_id);
        }
    }
    if (pending_.empty()) {
        ready_ = true;
        emit updated();
        return;
    }
    timer_->start();
}

void PropertyCatalog::scanChunk() {
    const size_t count = std::min<size_t>(kChunkSize, pending_.size());
    for (size_t i = pending_.size() - count; i < pending_.size(); ++i) {
        const uint32_t object_id = pending_[i];
        for (const auto& sets : {data_access_.propertySets(object_id), data_access_.quantitySets(object_id)}) {
            for (const auto& [set_name, values] : sets) {
                const QString pset = QString::fromStdString(set_name);
                for (const auto& [property_name, value] : values) {
                    if (property_name == "id") continue; // the definition's instance id
                    const QString property = QString::fromStdString(property_name);
                    ++properties_[pset][property];
                    QStringList& samples = samples_[pset][property];
                    if (samples.size() < kMaxSamples) {
                        const QString text = sampleText(value);
                        if (!text.isEmpty() && !samples.contains(text)) samples.append(text);
                    }
                }
            }
        }
    }
    pending_.resize(pending_.size() - count);
    if (pending_.empty()) {
        timer_->stop();
        ready_ = true;
        emit updated();
    }
}

QStringList PropertyCatalog::propertySets() const {
    QList<QPair<int, QString>> ranked;
    for (auto it = properties_.cbegin(); it != properties_.cend(); ++it) {
        int elements = 0;
        for (int count : it.value()) elements = std::max(elements, count);
        ranked.append({elements, it.key()});
    }
    std::stable_sort(ranked.begin(), ranked.end(),
                     [](const auto& a, const auto& b) { return a.first > b.first; });
    QStringList names;
    for (const auto& entry : ranked) names.append(entry.second);
    return names;
}

} // namespace bonsaiviewer::services
