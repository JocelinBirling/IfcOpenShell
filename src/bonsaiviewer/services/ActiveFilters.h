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

#ifndef IFCINTERFACE_SERVICES_ACTIVEFILTERS_H
#define IFCINTERFACE_SERVICES_ACTIVEFILTERS_H

#include "FilterStore.h"

#include <QByteArray>
#include <QObject>
#include <QStringList>

namespace bonsaiviewer { class SessionState; }

namespace bonsaiviewer::services {

class ActionPipeline;
class FilterRunner;

// BonsaiViewer06 Activate / Deactivate. The colours of the activated
// filters are forced: they are applied in the forced layer of B04, above
// every other colour, and kept by Restore View and by the runs of other
// filters. They are applied again (forced runner, one filter after the
// other) when the set of activated filters or one of them changes, and when
// a model is loaded. Only a filter without view action (Isolate, Add to the
// view, Remove from the view) can be activated.
class ActiveFilters : public QObject {
    Q_OBJECT

public:
    ActiveFilters(SessionState& session, FilterStore& store, FilterRunner& forced_runner,
                  ActionPipeline& pipeline, QObject* parent = nullptr);

    // Empty when the filter can be activated, else why not.
    static QString activationError(const FilterDocument& document);

signals:
    // A pass over the activated filters ended (`summary` for the status bar).
    void applied(const QString& summary);

private:
    // Applies the activated filters again when they changed (`force`: always).
    void schedule(bool force);
    void start();
    void runNext();

    SessionState& session_;
    FilterStore& store_;
    FilterRunner& runner_;
    ActionPipeline& pipeline_;
    QByteArray applied_state_; // the activated filters of the last pass
    QStringList pending_;      // the filters still to run in this pass
    bool scheduled_ = false;
    bool forced_pass_ = false;
    bool restart_ = false;     // a change arrived during a pass
    int applied_count_ = 0;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_ACTIVEFILTERS_H
