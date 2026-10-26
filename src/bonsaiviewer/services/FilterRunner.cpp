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

#include "FilterRunner.h"

#include "ActionPipeline.h"
#include "../ElementRegistry.h"
#include "EntitySetStore.h"
#include "IdsBridge.h"
#include "../SessionState.h"

#include "../../ifcviewer/SceneLoader.h"

#include <QFileInfo>
#include <QHash>

#include <algorithm>

namespace bonsaiviewer::services {

namespace {

void mergeInto(std::vector<int>& target, const std::vector<int>& source) {
    std::vector<int> merged;
    merged.reserve(target.size() + source.size());
    std::set_union(target.begin(), target.end(), source.begin(), source.end(),
                   std::back_inserter(merged));
    target = std::move(merged);
}

} // namespace

FilterRunner::FilterRunner(SessionState& session,
                           IdsBridge& ids_bridge,
                           ActionPipeline& pipeline,
                           EntitySetStore& set_store,
                           QObject* parent)
    : QObject(parent),
      session_(session),
      ids_bridge_(ids_bridge),
      pipeline_(pipeline),
      set_store_(set_store)
{
}

QString FilterRunner::activeIfcPath(SessionState& session, uint32_t* session_model_id) {
    const QString active_model_id = session.activeModelId();
    const uint32_t model_id = active_model_id.isEmpty()
        ? 0
        : session.sessionModelIdForModelId(active_model_id);
    if (session_model_id != nullptr) *session_model_id = model_id;
    auto* loader = session.loader();
    return (loader != nullptr && model_id != 0) ? loader->filePath(model_id) : QString();
}

bool FilterRunner::run(const FilterDocument& document, Scope scope) {
    if (running_) {
        emit finished(false, "a filter is already running");
        return false;
    }
    uint32_t session_model_id = 0;
    const QString ifc_path = activeIfcPath(session_, &session_model_id);
    if (ifc_path.isEmpty()) {
        emit finished(false, "no active IFC model");
        return false;
    }
    if (!QFileInfo::exists(ifc_path)) {
        emit finished(false, QString("the IFC file of the active model cannot be found: %1").arg(ifc_path));
        return false;
    }
    if (document.layers.empty()) {
        emit finished(false, "the filter has no layers");
        return false;
    }

    // Run on the box: the candidates are the box's EXPRESS ids in the
    // active model, computed once for the whole run.
    std::optional<std::vector<int>> limit;
    if (scope == Scope::Box) {
        std::vector<int> ids;
        for (uint32_t object_id : set_store_.current()) {
            const auto info = session_.elementRegistry()->findBasicElementInfo(object_id);
            if (info && info->session_model_id == session_model_id && info->ifc_id != 0) {
                ids.push_back(info->ifc_id);
            }
        }
        if (ids.empty()) {
            emit finished(false, "the box is empty (or holds no element of the active model)");
            return false;
        }
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
        limit = std::move(ids);
    }

    document_ = document;
    scope_ = scope;
    limit_ = std::move(limit);
    layer_index_ = 0;
    ifc_path_ = ifc_path;
    session_model_id_ = session_model_id;
    pending_.clear();
    last_group_.clear();
    group_open_ = false;
    actioned_elements_ = 0;
    view_cleared_ = false;
    layers_executed_ = 0;
    running_ = true;
    step();
    return true;
}

void FilterRunner::step() {
    if (layer_index_ >= document_.layers.size()) {
        running_ = false;
        emit finished(true, QString("%1 layer(s) run, %2 element(s) affected")
                                .arg(layers_executed_)
                                .arg(actioned_elements_));
        return;
    }
    const FilterLayer& layer = document_.layers[layer_index_];
    if (!layer.is_and && !layer.ids_xml.isEmpty()) {
        emit progress(QString("selecting layer %1 of %2…")
                          .arg(layer_index_ + 1)
                          .arg(static_cast<int>(document_.layers.size())));
        ids_bridge_.select(ifc_path_, layer.ids_xml, limit_,
                           [this](bool ok, std::vector<int> ids, const QString& error) {
            if (!ok) {
                fail(error);
                return;
            }
            std::sort(ids.begin(), ids.end());
            mergeInto(pending_, ids);
            group_open_ = true;
            finishLayer();
        });
        return;
    }
    // An AND layer (or a specification-less layer): reuse what is above.
    // Queued, so a long filter of AND layers does not recurse.
    QMetaObject::invokeMethod(this, &FilterRunner::finishLayer, Qt::QueuedConnection);
}

void FilterRunner::finishLayer() {
    const FilterLayer& layer = document_.layers[layer_index_];
    if (!layer.actions.empty()) {
        // The pending set holds the selections accumulated since the last
        // action, including this layer's own specification when it has
        // one; an AND layer placed right after an action reuses the last
        // executed group instead.
        // An open group may legitimately be empty (its specifications
        // matched nothing): it must not fall back to the previous group.
        const std::vector<int> effective = group_open_ ? pending_ : last_group_;
        applyActions(layer, effective);
        last_group_ = effective;
        pending_.clear();
        group_open_ = false;
    }
    ++layers_executed_;
    ++layer_index_;
    step();
}

void FilterRunner::fail(const QString& error) {
    running_ = false;
    pending_.clear();
    last_group_.clear();
    emit finished(false, QString("layer %1: %2").arg(layer_index_ + 1).arg(error));
}

void FilterRunner::applyActions(const FilterLayer& layer, const std::vector<int>& express_ids) {
    // EXPRESS ids -> the session's object ids through the element registry;
    // ids of other models simply do not resolve.
    QHash<int, uint32_t> object_id_by_ifc_id;
    for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id_)) {
        object_id_by_ifc_id.insert(info.ifc_id, info.object_id);
    }
    std::vector<uint32_t> object_ids;
    object_ids.reserve(express_ids.size());
    for (int id : express_ids) {
        const auto found = object_id_by_ifc_id.find(id);
        if (found != object_id_by_ifc_id.end()) object_ids.push_back(found.value());
    }
    // A filter starts from an empty view: everything is hidden right before
    // its first action (not at launch, so a failed selection leaves the
    // view untouched). An action layer that matches nothing still empties
    // the view.
    if (!view_cleared_) {
        pipeline_.run({}, ActionPipeline::Action::Isolate);
        view_cleared_ = true;
    }
    if (object_ids.empty()) return;
    actioned_elements_ += static_cast<int>(object_ids.size());
    for (const auto& action : layer.actions) {
        switch (action.type) {
        case FilterAction::Type::Isolate:
            pipeline_.run(object_ids, ActionPipeline::Action::Isolate);
            break;
        case FilterAction::Type::AddToView:
            pipeline_.run(object_ids, ActionPipeline::Action::AddToView);
            break;
        case FilterAction::Type::RemoveFromView:
            pipeline_.run(object_ids, ActionPipeline::Action::RemoveFromView);
            break;
        case FilterAction::Type::Colour:
            // Show the elements, then paint them (like the Paint bucket).
            pipeline_.run(object_ids, ActionPipeline::Action::AddToView);
            pipeline_.colour(object_ids, action.colour);
            break;
        case FilterAction::Type::Transparency:
            pipeline_.transparency(object_ids, action.transparency_percent);
            break;
        case FilterAction::Type::AutoColourByProperty:
            // Show the elements, then colour them.
            pipeline_.run(object_ids, ActionPipeline::Action::AddToView);
            pipeline_.autoColourByProperty(object_ids,
                                           action.pset.toStdString(),
                                           action.property.toStdString());
            break;
        }
    }
}

} // namespace bonsaiviewer::services
