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

#ifndef IFCINTERFACE_MODULES_IDS_VIEW_H
#define IFCINTERFACE_MODULES_IDS_VIEW_H

#include "IdsDocument.h"

#include <QObject>

#include <vector>

namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::components { class Panel; }
namespace bonsaiviewer::services {
class ActionPipeline;
class AppearanceService;
class IdsBridge;
}

namespace bonsaiviewer::modules::ids {

class IdsEditor;

// BonsaiViewer24 — drives the IDS panel: builds its editor, runs the IDS on
// every loaded model (C++ port of IfcTester, on the files the viewer has
// open) and isolates the elements picked in the results.
//
// A specification is judged on the whole federation: its applicable,
// passing and failing elements are those of all the loaded models of a
// listed IFC version; "required" fails when none of them holds an
// applicable element, "prohibited" when any does.
class IdsPanelView : public QObject {
    Q_OBJECT

public:
    IdsPanelView(bonsaiviewer::components::Panel* panel,
                 bonsaiviewer::SessionState& session,
                 bonsaiviewer::services::IdsBridge& bridge,
                 bonsaiviewer::services::ActionPipeline& pipeline,
                 bonsaiviewer::services::AppearanceService& appearance,
                 QObject* parent = nullptr);

    IdsEditor* editor() const { return editor_; }

private:
    void run();
    void isolate(const std::vector<ElementRef>& elements);
    void colour(const std::vector<ElementRef>& passed, const std::vector<ElementRef>& failed);
    // The object ids of the elements that have geometry in the viewer.
    std::vector<uint32_t> objectIds(const std::vector<ElementRef>& elements) const;

    bonsaiviewer::SessionState& session_;
    bonsaiviewer::services::IdsBridge& bridge_;
    bonsaiviewer::services::ActionPipeline& pipeline_;
    bonsaiviewer::services::AppearanceService& appearance_;
    IdsEditor* editor_ = nullptr;
};

} // namespace bonsaiviewer::modules::ids

#endif // IFCINTERFACE_MODULES_IDS_VIEW_H
