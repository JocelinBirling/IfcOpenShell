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

#include "FilterStore.h"

#include <QObject>
#include <QString>

#include <optional>
#include <vector>

namespace bonsaiviewer {
class SessionState;
}

namespace bonsaiviewer::services {
class ActionPipeline;
class EntitySetStore;
class IdsBridge;
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
// The worked example of the specification is the acceptance test. The
// selections come from the IdsBridge (IfcTester); the actions run through
// B08 ActionPipeline. Runs are asynchronous: the runner emits progress()
// per stage and finished() with a summary once done.

namespace bonsaiviewer::services {

class FilterRunner : public QObject {
    Q_OBJECT

public:
    enum class Scope {
        WholeModel,
        Box, // the current EntitySetStore set instead of the whole model
    };

    FilterRunner(SessionState& session,
                 IdsBridge& ids_bridge,
                 ActionPipeline& pipeline,
                 EntitySetStore& set_store,
                 QObject* parent = nullptr);

    // Starts an asynchronous run. Returns false (and emits finished) when
    // there is nothing to run on — no active model, or an empty document.
    bool run(const FilterDocument& document, Scope scope);

    bool isRunning() const { return running_; }

    // The IFC file of the active model (empty when none is loaded), and
    // optionally its session model id. Shared with the creation window's
    // live preview.
    static QString activeIfcPath(SessionState& session, uint32_t* session_model_id = nullptr);

signals:
    void progress(const QString& stage);
    void finished(bool ok, const QString& summary);

private:
    void step();
    void finishLayer();
    void fail(const QString& error);
    void applyActions(const FilterLayer& layer, const std::vector<int>& express_ids);

    SessionState& session_;
    IdsBridge& ids_bridge_;
    ActionPipeline& pipeline_;
    EntitySetStore& set_store_;

    // Run state.
    bool running_ = false;
    FilterDocument document_;
    Scope scope_ = Scope::WholeModel;
    std::optional<std::vector<int>> limit_; // EXPRESS ids of the box (Scope::Box)
    size_t layer_index_ = 0;
    QString ifc_path_;
    uint32_t session_model_id_ = 0;
    std::vector<int> pending_;     // express ids accumulated since the last action
    std::vector<int> last_group_;  // express ids the previous action group ran on
    bool group_open_ = false;      // a specification layer ran since the last action
    int actioned_elements_ = 0;
    bool view_cleared_ = false;    // the view was emptied before the first action
    int layers_executed_ = 0;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_FILTERRUNNER_H
