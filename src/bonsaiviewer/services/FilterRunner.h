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

#ifndef IFCINTERFACE_SERVICES_FILTERRUNNER_H
#define IFCINTERFACE_SERVICES_FILTERRUNNER_H

#include "ActionPipeline.h"
#include "FilterStore.h"

#include <QColor>
#include <QObject>
#include <QString>

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace bonsaiviewer {
class SessionState;
}

namespace bonsaiviewer::services {
class ActionPipeline;
class EntitySetStore;
class IdsBridge;
class UndoStack;
}

// B06 filters — FilterRunner. Walks the layers of a filter document from
// top to bottom, with the pending set semantics of the closed spec:
//
//   * a layer with a specification and no action unions its elements into
//     the pending set;
//   * an AND layer adds nothing: it reuses the selection of the layer
//     above (the open group, or the last executed group when an action
//     layer is right above);
//   * a layer's actions apply to the pending set (plus the layer's own
//     selection when it has one), then the pending set is emptied and a
//     new group starts;
//   * a run starts from an empty view: everything is hidden right before
//     the first action.
//
// A run applies the filter to every loaded model (BonsaiViewer06). The
// layers run once per model, one model after the other; the view is
// emptied once for the whole run. EXPRESS ids are per file, so each model
// keeps its own pending set.
//
// The selections come from the IdsBridge (IfcTester); the actions run
// through B08 ActionPipeline. Runs are asynchronous: the runner emits
// progress() per stage and finished() with a summary once done.

namespace bonsaiviewer::services {

class FilterRunner : public QObject {
    Q_OBJECT

public:
    enum class Scope {
        WholeModel, // every loaded model, whole content
        Box,        // every loaded model, only the elements of the current box
    };

    FilterRunner(SessionState& session,
                 IdsBridge& ids_bridge,
                 ActionPipeline& pipeline,
                 EntitySetStore& set_store,
                 QObject* parent = nullptr);

    // Starts an asynchronous run. Returns false (and emits finished) when
    // there is nothing to run on — no loaded model, or an empty document.
    bool run(const FilterDocument& document, Scope scope);

    bool isRunning() const { return running_; }

    // Set by the shell. A run suspends the undo stack from its start to its
    // end, so filters never store an undo point (BonsaiViewer27).
    void setUndoStack(UndoStack* undo) { undo_ = undo; }

    // The IFC file of the active model (empty when none is loaded), and
    // optionally its session model id. Shared with the creation window's
    // live preview.
    static QString activeIfcPath(SessionState& session, uint32_t* session_model_id = nullptr);

signals:
    void progress(const QString& stage);
    void finished(bool ok, const QString& summary);
    // An Automatic Color action ran (B06): the shell shows its temporary table.
    // `relation` is set when the colour follows a relation (pset and
    // property are then empty).
    void autoColoured(const QString& pset, const QString& property, const QString& relation,
                      const std::vector<std::uint32_t>& ids,
                      const std::unordered_map<std::uint32_t, QColor>& colours);

private:
    // One loaded model the filter runs on.
    struct Target {
        QString ifc_path;
        uint32_t session_model_id = 0;
        std::optional<std::vector<int>> limit; // EXPRESS ids of the box (Scope::Box)
    };

    void startTarget();
    void step();
    void finishLayer();
    void fail(const QString& error);
    void applyActions(const FilterLayer& layer, const std::vector<int>& express_ids);
    // Automatic Color: the values collected on every model of the run,
    // coloured at the end of the run; returns a summary for `finished`.
    struct AutoColourRun {
        QString pset;
        QString property;
        QString relation;
        ActionPipeline::ElementValues values;
    };
    std::vector<AutoColourRun> auto_colour_runs_;
    QString colourAutoColourRuns();
    // Automatic Color: pset.property of each selected element, read in the
    // selection's file. nullopt when that file is not available.
    // Automatic Color by relation: the related names of each element (see
    // RelationValues), read in the selection's file.
    std::optional<ActionPipeline::ElementValues> relationValues(const std::vector<int>& express_ids,
                                                                const QString& relation);
    std::optional<ActionPipeline::ElementValues> propertyValues(const std::vector<int>& express_ids,
                                                                const std::string& pset,
                                                                const std::string& property);

    SessionState& session_;
    IdsBridge& ids_bridge_;
    ActionPipeline& pipeline_;
    EntitySetStore& set_store_;

    // Run state.
    bool running_ = false;
    FilterDocument document_;
    Scope scope_ = Scope::WholeModel;
    std::vector<Target> targets_;
    size_t target_index_ = 0;      // model being run
    size_t layer_index_ = 0;       // layer of that model
    std::vector<int> pending_;     // express ids accumulated since the last action
    std::vector<int> last_group_;  // express ids the previous action group ran on
    bool group_open_ = false;      // a specification layer ran since the last action
    int actioned_elements_ = 0;
    bool view_cleared_ = false;    // the view was emptied before the first action
    int layers_executed_ = 0;
    // An Isolate action replaces the view, so it must keep the elements the
    // previous models of this run have shown. kept_ids_ holds those from the
    // finished models; target_shown_ the ones shown by the current model.
    std::vector<uint32_t> kept_ids_;
    std::vector<uint32_t> target_shown_;
    // EXPRESS id -> object id of the current model, built once per model.
    std::unordered_map<int, uint32_t> object_id_by_ifc_id_;
    UndoStack* undo_ = nullptr;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_FILTERRUNNER_H
