// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_PANELS_MODELSPANELVIEW_H
#define IFCINTERFACE_PANELS_MODELSPANELVIEW_H

#include "Types.h"

#include <QObject>
#include <QStringList>

#include <cstdint>
#include <vector>

class Federation;
class ViewportWindow;
namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::services {
class ActiveView;
class AppearanceService;
class SelectionService;
}

namespace bonsaiviewer::modules::models {

class FederationItemModel;
class ModelsPanel;

// Pure derivation used by ModelsPanel when building its "move to..." menus.
// Walks the federation's group tree and returns every group except those in
// the subtree rooted at exclude_subtree_root (skip a group's own subtree to
// prevent a cyclic move). Pass an empty exclude_subtree_root to get every
// group back.
QList<GroupOption> validMoveTargets(const Federation& federation,
                                    const QString& exclude_subtree_root);

// Owns the FederationItemModel, hands it to the panel, and listens to the
// coarse session signals (project open/reset, theme change) — those are the
// "rebuild from scratch" cases the model itself doesn't subscribe to.
// Granular Federation events are handled inside the model.
//
// Also the bridge for the one thing the tree shows that is not Federation
// state: each model's GPU residency (memory column, unloaded styling). The
// viewport owns that state, so this view polls it once a second — the
// numbers move continuously while geometry streams — and pushes it in.
class ModelsPanelView : public QObject {
    Q_OBJECT
public:
    explicit ModelsPanelView(ModelsPanel* widget,
                             bonsaiviewer::SessionState* session_state,
                             ViewportWindow* viewport,
                             QObject* parent = nullptr);

    // Set once the colour service exists (created after the panels).
    void setAppearanceService(bonsaiviewer::services::AppearanceService* service);
    // Set once the view and selection services exist (BonsaiViewer29).
    void setModelServices(bonsaiviewer::services::ActiveView* active_view,
                          bonsaiviewer::services::SelectionService* selection);

private:
    void refreshResidency();
    // Colours every element of the given models with a colour picked by the user.
    void colourModels(const QStringList& model_ids);
    // Element ids of the given models (hidden ones included).
    std::vector<uint32_t> objectIdsOf(const QStringList& model_ids) const;
    void isolateModels(const QStringList& model_ids);
    // Selects the visible elements of the given models.
    void selectModels(const QStringList& model_ids);

    ModelsPanel* widget_ = nullptr;
    bonsaiviewer::SessionState* session_state_ = nullptr;
    ViewportWindow* viewport_ = nullptr;
    FederationItemModel* model_ = nullptr;
    bonsaiviewer::services::AppearanceService* appearance_ = nullptr;
    bonsaiviewer::services::ActiveView* active_view_ = nullptr;
    bonsaiviewer::services::SelectionService* selection_ = nullptr;
};

} // namespace bonsaiviewer::modules::models

#endif
