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

#include "ActiveFilters.h"

#include "ActionPipeline.h"
#include "FilterRunner.h"
#include "../SessionState.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#include <functional>

namespace bonsaiviewer::services {

namespace {

// Every filter of the store, folders included, in one list.
std::vector<FilterDocument> allFilters(const FilterStore& store) {
    std::vector<FilterDocument> filters;
    std::function<void(const QString&)> walk = [&](const QString& folder_id) {
        for (const auto& document : store.filtersInFolder(folder_id)) filters.push_back(document);
        for (const auto& folder : store.folders()) {
            if (folder.parent_id == folder_id) walk(folder.id);
        }
    };
    walk({});
    return filters;
}

std::vector<FilterDocument> activeFilters(const FilterStore& store) {
    std::vector<FilterDocument> active;
    for (const auto& document : allFilters(store)) {
        if (document.active && ActiveFilters::activationError(document).isEmpty()) active.push_back(document);
    }
    return active;
}

QByteArray stateOf(const std::vector<FilterDocument>& filters) {
    QJsonArray array;
    for (const auto& document : filters) array.append(FilterStore::documentToJson(document));
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

} // namespace

ActiveFilters::ActiveFilters(SessionState& session, FilterStore& store, FilterRunner& forced_runner,
                             ActionPipeline& pipeline, QObject* parent)
    : QObject(parent), session_(session), store_(store), runner_(forced_runner), pipeline_(pipeline) {
    connect(&store_, &FilterStore::changed, this, [this]() { schedule(false); });
    // A (re)loaded model lost its colours (B04 purges them): apply again.
    connect(&session_, &SessionState::modelGeometryReady, this, [this](std::uint32_t) { schedule(true); });
    // Queued: a run with nothing to do emits finished from inside run().
    connect(&runner_, &FilterRunner::finished, this, [this](bool, const QString&) { runNext(); },
            Qt::QueuedConnection);
}

QString ActiveFilters::activationError(const FilterDocument& document) {
    bool colours = false;
    for (const auto& layer : document.layers) {
        for (const auto& action : layer.actions) {
            switch (action.type) {
            case FilterAction::Type::Isolate:
            case FilterAction::Type::AddToView:
            case FilterAction::Type::RemoveFromView:
                return "An activated filter only colours: remove its Isolate, Add to the view and Remove "
                       "from the view actions to activate it.";
            case FilterAction::Type::Colour:
            case FilterAction::Type::Transparency:
            case FilterAction::Type::AutoColourByProperty:
                colours = true;
                break;
            }
        }
    }
    if (!colours) return "The filter has no Colour, Transparency or Automatic Color action.";
    return {};
}

void ActiveFilters::schedule(bool force) {
    forced_pass_ = forced_pass_ || force;
    if (scheduled_) return;
    scheduled_ = true;
    // Coalesces the changes of one event (a save emits several signals).
    QTimer::singleShot(0, this, &ActiveFilters::start);
}

void ActiveFilters::start() {
    scheduled_ = false;
    if (runner_.isRunning() || !pending_.isEmpty()) {
        restart_ = true; // after the current pass
        return;
    }
    const auto active = activeFilters(store_);
    const QByteArray state = stateOf(active);
    if (!forced_pass_ && state == applied_state_) return;
    forced_pass_ = false;
    applied_state_ = state;
    pipeline_.clearForced();
    applied_count_ = 0;
    for (const auto& document : active) pending_.append(document.id);
    runNext();
}

void ActiveFilters::runNext() {
    while (!pending_.isEmpty()) {
        const QString id = pending_.takeFirst();
        const auto document = store_.filter(id);
        if (!document) continue;
        ++applied_count_;
        // finished() (also emitted when there is nothing to run on) calls runNext.
        runner_.run(*document, FilterRunner::Scope::WholeModel);
        return;
    }
    if (applied_count_ > 0) emit applied(QString("%1 activated filter(s) applied").arg(applied_count_));
    applied_count_ = 0;
    if (restart_) {
        restart_ = false;
        schedule(true);
    }
}

} // namespace bonsaiviewer::services
