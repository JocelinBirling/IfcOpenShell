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
#include "UndoStack.h"
#include "../SessionState.h"

#include "../../ifcviewer/SceneLoader.h"

#include "pset.h"

#include <QFileInfo>
#include <QHash>

#include <algorithm>
#include <sstream>
#include <unordered_set>

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
    if (document.layers.empty()) {
        emit finished(false, "the filter has no layers");
        return false;
    }

    // Every loaded model, as the filter applies to the whole federation.
    std::vector<Target> targets;
    auto* loader = session_.loader();
    for (uint32_t session_model_id : session_.sessionModelIds()) {
        if (loader == nullptr || session_model_id == 0) continue;
        const QString path = loader->filePath(session_model_id);
        if (path.isEmpty() || !QFileInfo::exists(path)) continue;
        Target target;
        target.ifc_path = path;
        target.session_model_id = session_model_id;
        targets.push_back(std::move(target));
    }
    if (targets.empty()) {
        emit finished(false, "no loaded IFC model");
        return false;
    }

    // Run on the box: per model, the candidates are the box's EXPRESS ids,
    // computed once for the whole run. Models without box elements are skipped.
    if (scope == Scope::Box) {
        for (auto& target : targets) {
            std::vector<int> ids;
            for (uint32_t object_id : set_store_.current()) {
                const auto info = session_.elementRegistry()->findBasicElementInfo(object_id);
                if (info && info->session_model_id == target.session_model_id && info->ifc_id != 0) {
                    ids.push_back(info->ifc_id);
                }
            }
            if (ids.empty()) continue;
            std::sort(ids.begin(), ids.end());
            ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
            target.limit = std::move(ids);
        }
        targets.erase(std::remove_if(targets.begin(), targets.end(),
                                     [](const Target& t) { return !t.limit.has_value(); }),
                      targets.end());
        if (targets.empty()) {
            emit finished(false, "the box is empty (or holds no element of the loaded models)");
            return false;
        }
    }

    document_ = document;
    scope_ = scope;
    targets_ = std::move(targets);
    target_index_ = 0;
    actioned_elements_ = 0;
    view_cleared_ = false;
    layers_executed_ = 0;
    kept_ids_.clear();
    auto_colour_runs_.clear();
    target_shown_.clear();
    running_ = true;
    if (undo_) undo_->setSuspended(true);
    startTarget();
    return true;
}

void FilterRunner::startTarget() {
    // The previous model is done: its shown elements are kept for the next
    // Isolate action of this run.
    kept_ids_.insert(kept_ids_.end(), target_shown_.begin(), target_shown_.end());
    target_shown_.clear();
    object_id_by_ifc_id_.clear();
    if (target_index_ < targets_.size()) {
        const auto elements =
            session_.elementRegistry()->basicElementInfoForModel(targets_[target_index_].session_model_id);
        object_id_by_ifc_id_.reserve(elements.size());
        for (const auto& info : elements) object_id_by_ifc_id_.emplace(info.ifc_id, info.object_id);
    }
    layer_index_ = 0;
    pending_.clear();
    last_group_.clear();
    group_open_ = false;
    step();
}

void FilterRunner::step() {
    if (layer_index_ >= document_.layers.size()) {
        // This model is done: the next loaded model starts from its first layer.
        if (target_index_ + 1 < targets_.size()) {
            ++target_index_;
            startTarget();
            return;
        }
        running_ = false;
        const QString auto_colour = colourAutoColourRuns();
        if (undo_) undo_->setSuspended(false);
        emit finished(true, QString("%1 layer(s) run on %2 model(s), %3 element(s) affected%4")
                                .arg(layers_executed_)
                                .arg(targets_.size())
                                .arg(actioned_elements_)
                                .arg(auto_colour));
        return;
    }
    const FilterLayer& layer = document_.layers[layer_index_];
    if (!layer.is_and && !layer.ids_xml.isEmpty()) {
        const Target& target = targets_[target_index_];
        emit progress(QString("model %1 of %2, selecting layer %3 of %4…")
                          .arg(target_index_ + 1)
                          .arg(targets_.size())
                          .arg(layer_index_ + 1)
                          .arg(static_cast<int>(document_.layers.size())));
        ids_bridge_.select(target.ifc_path, layer.ids_xml, target.limit,
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
    if (undo_) undo_->setSuspended(false);
    pending_.clear();
    last_group_.clear();
    emit finished(false, QString("model %1, layer %2: %3")
                             .arg(target_index_ + 1)
                             .arg(layer_index_ + 1)
                             .arg(error));
}

QString FilterRunner::colourAutoColourRuns() {
    QString summary;
    for (const AutoColourRun& run : auto_colour_runs_) {
        const auto colours = pipeline_.autoColourByValues(run.values);
        std::vector<std::uint32_t> ids;
        ids.reserve(run.values.size());
        std::size_t found = 0;
        for (const auto& [id, value] : run.values) {
            ids.push_back(id);
            if (value) ++found;
        }
        emit autoColoured(run.pset, run.property, ids, colours);
        summary += QString("; Automatic Color %1.%2: %3 of %4 element(s) with a value")
                       .arg(run.pset, run.property)
                       .arg(static_cast<qulonglong>(found))
                       .arg(static_cast<qulonglong>(ids.size()));
    }
    auto_colour_runs_.clear();
    return summary;
}

std::optional<ActionPipeline::ElementValues> FilterRunner::propertyValues(
    const std::vector<int>& express_ids, const std::string& pset, const std::string& property) {
    if (target_index_ >= targets_.size()) return std::nullopt;
    ifcopenshell::file* file = ids_bridge_.file(targets_[target_index_].ifc_path);
    if (file == nullptr) return std::nullopt;
    auto find = [&pset, &property](const element_properties& sets) -> std::optional<std::string> {
        const auto set = sets.find(pset);
        if (set == sets.end()) return std::nullopt;
        const auto value = set->second.find(property);
        if (value == set->second.end() || property == "id") return std::nullopt;
        std::ostringstream formatted;
        formatted << value->second;
        return formatted.str();
    };
    ActionPipeline::ElementValues values;
    values.reserve(express_ids.size());
    for (int express_id : express_ids) {
        const auto object = object_id_by_ifc_id_.find(express_id);
        if (object == object_id_by_ifc_id_.end()) continue;
        std::optional<std::string> value;
        try {
            const express::base element = file->instance_by_id(express_id);
            value = find(get_psets(element, /*psets_only=*/true));
            if (!value) value = find(get_psets(element, /*psets_only=*/false, /*qtos_only=*/true));
        } catch (...) {
        }
        values.emplace_back(object->second, std::move(value));
    }
    return values;
}

void FilterRunner::applyActions(const FilterLayer& layer, const std::vector<int>& express_ids) {
    // EXPRESS ids -> the session's object ids (map built once per model).
    std::vector<uint32_t> object_ids;
    object_ids.reserve(express_ids.size());
    for (int id : express_ids) {
        const auto found = object_id_by_ifc_id_.find(id);
        if (found != object_id_by_ifc_id_.end()) object_ids.push_back(found->second);
    }
    // A filter starts from an empty view: everything is hidden right before
    // its first action (not at launch, so a failed selection leaves the
    // view untouched). Once per run, not once per model.
    if (!view_cleared_) {
        pipeline_.run({}, ActionPipeline::Action::Isolate);
        view_cleared_ = true;
    }
    if (object_ids.empty()) return;
    actioned_elements_ += static_cast<int>(object_ids.size());
    for (const auto& action : layer.actions) {
        switch (action.type) {
        case FilterAction::Type::Isolate: {
            // Replaces the view, but keeps the elements of the previous
            // models of this run (BonsaiViewer06: the filter covers every model).
            std::vector<uint32_t> isolated = kept_ids_;
            isolated.insert(isolated.end(), object_ids.begin(), object_ids.end());
            std::sort(isolated.begin(), isolated.end());
            isolated.erase(std::unique(isolated.begin(), isolated.end()), isolated.end());
            pipeline_.run(isolated, ActionPipeline::Action::Isolate);
            target_shown_.insert(target_shown_.end(), object_ids.begin(), object_ids.end());
            break;
        }
        case FilterAction::Type::AddToView:
            pipeline_.run(object_ids, ActionPipeline::Action::AddToView);
            target_shown_.insert(target_shown_.end(), object_ids.begin(), object_ids.end());
            break;
        case FilterAction::Type::RemoveFromView: {
            pipeline_.run(object_ids, ActionPipeline::Action::RemoveFromView);
            // Hash set: a linear search per element was quadratic on large sets.
            const std::unordered_set<uint32_t> removed(object_ids.begin(), object_ids.end());
            target_shown_.erase(std::remove_if(target_shown_.begin(), target_shown_.end(),
                                               [&removed](uint32_t id) { return removed.count(id) != 0; }),
                                target_shown_.end());
            break;
        }
        case FilterAction::Type::Colour:
            // Colour only: the elements are not shown or hidden by this action.
            // They must be added with an Add to the view action to be displayed.
            pipeline_.colour(object_ids, action.colour);
            break;
        case FilterAction::Type::Transparency:
            pipeline_.transparency(object_ids, action.transparency_percent);
            break;
        case FilterAction::Type::AutoColourByProperty:
            // Show the elements, then colour them.
            pipeline_.run(object_ids, ActionPipeline::Action::AddToView);
            target_shown_.insert(target_shown_.end(), object_ids.begin(), object_ids.end());
        {
            // The values are read in the file the selection ran on (always
            // open), else in the viewer's file. The colours are given at the
            // end of the run, on every model at once, so that one value has
            // one colour across the models (see colourAutoColourRuns).
            const std::string pset = action.pset.toStdString();
            const std::string property = action.property.toStdString();
            auto values = propertyValues(express_ids, pset, property);
            if (!values) values = pipeline_.readValues(object_ids, pset, property);
            auto run = std::find_if(auto_colour_runs_.begin(), auto_colour_runs_.end(),
                                    [&action](const AutoColourRun& r) {
                                        return r.pset == action.pset && r.property == action.property;
                                    });
            if (run == auto_colour_runs_.end()) {
                auto_colour_runs_.push_back({action.pset, action.property, {}});
                run = std::prev(auto_colour_runs_.end());
            }
            run->values.insert(run->values.end(), values->begin(), values->end());
            break;
        }
        }
    }
}

} // namespace bonsaiviewer::services
