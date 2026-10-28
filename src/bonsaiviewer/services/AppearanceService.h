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
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_SERVICES_APPEARANCESERVICE_H
#define IFCINTERFACE_SERVICES_APPEARANCESERVICE_H

#include <QColor>
#include <QObject>

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

class ViewportWindow;

namespace bonsaiviewer { class SessionState; }

// B04 — AppearanceService (see 00_Technical_Implementation/
// BonsaiViewerTI00). Per-object colour and opacity overrides with priority
// layers: forced filter > user paint > table colour > IDS result > filter
// action > clash run. The
// effective override is the RGB of the highest-priority layer plus the
// opacity byte; an opacity without any colour layer keeps the baked vertex
// colour through the viewport's alpha-only flag. Removing the user paint
// restores the baked colour while keeping the opacity (TI02). All overrides
// of a model are reset when its geometry is (re)loaded, and everything is
// cleared when the project is reset.

namespace bonsaiviewer::services {

class UndoStack;

class AppearanceService : public QObject {
    Q_OBJECT

public:
    // Priority order: a lower enum value wins.
    enum class Layer {
        // BonsaiViewer06: the colours of the activated filters. They win
        // over every other layer and survive clearOverrides().
        ForcedFilter = 0,
        UserPaint = 1,
        TableColour = 2,
        IdsResult = 3, // BonsaiViewer24: passing green, failing red
        FilterAction = 4,
        ClashRun = 5,
    };
    static constexpr int kLayerCount = 6;

    AppearanceService(bonsaiviewer::SessionState& session,
                      ViewportWindow& viewport,
                      QObject* parent = nullptr);

    // Paint / unpaint in a given layer. setColour defaults to the user
    // paint layer (the ribbon Paint / Remove Color of TI02).
    void setColour(const std::vector<std::uint32_t>& ids,
                   const QColor& colour, Layer layer = Layer::UserPaint);
    void removeColour(const std::vector<std::uint32_t>& ids);
    void setLayerColour(const std::vector<std::uint32_t>& ids,
                        const QColor& colour, Layer layer);
    void clearLayer(Layer layer);

    // 100 restores the original (baked) opacity; colour layers are kept.
    // The objects with a forced opacity keep it.
    void setOpacity(const std::vector<std::uint32_t>& ids, int percent);

    // BonsaiViewer06 activated filters: an opacity no other call changes.
    void setForcedOpacity(const std::vector<std::uint32_t>& ids, int percent);
    // Drops the forced colours and opacities (before the activated filters
    // are applied again, or when they are all deactivated).
    void clearForced();

    // Drop every override except the forced ones (activated filters) and
    // restore the viewport.
    void clearOverrides();
    // Drop every override, the forced ones included (project reset).
    void clearAll();

    // Undo (BonsaiViewer27): the overrides as they are now, and a way back.
    struct ObjectState {
        bool        layer_set[kLayerCount] = {};
        std::uint32_t layer_rgb8[kLayerCount] = {};
        std::uint8_t opacity = 255;
    };
    using Snapshot = std::unordered_map<std::uint32_t, ObjectState>;
    Snapshot snapshot() const { return states_; }
    void restore(const Snapshot& snapshot);

    // Set by the shell. setColour and setOpacity record a display state
    // before they change anything (see UndoStack).
    void setUndoStack(UndoStack* undo) { undo_ = undo; }

signals:
    void appearanceChanged(const std::vector<std::uint32_t>& ids);

private:
    void applyToViewport(const std::vector<std::uint32_t>& ids);
    void purgeModel(std::uint32_t session_model_id);

    bonsaiviewer::SessionState& session_;
    ViewportWindow& viewport_;
    std::unordered_map<std::uint32_t, ObjectState> states_;
    // The opacities forced by the activated filters (alpha byte).
    std::unordered_map<std::uint32_t, std::uint8_t> forced_opacity_;
    UndoStack* undo_ = nullptr;
};

} // namespace bonsaiviewer::services

#endif
