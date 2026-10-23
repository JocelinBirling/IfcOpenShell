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
// layers: user paint > table colour > filter action > clash run. The
// effective override is the RGB of the highest-priority layer plus the
// opacity byte; an opacity without any colour layer keeps the baked vertex
// colour through the viewport's alpha-only flag. Removing the user paint
// restores the baked colour while keeping the opacity (TI02). All overrides
// of a model are reset when its geometry is (re)loaded, and everything is
// cleared when the project is reset.

namespace bonsaiviewer::services {

class AppearanceService : public QObject {
    Q_OBJECT

public:
    // Priority order: a lower enum value wins.
    enum class Layer {
        UserPaint = 0,
        TableColour = 1,
        FilterAction = 2,
        ClashRun = 3,
    };
    static constexpr int kLayerCount = 4;

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
    void setOpacity(const std::vector<std::uint32_t>& ids, int percent);

    // Drop every override in every layer and restore the viewport.
    void clearOverrides();

signals:
    void appearanceChanged(const std::vector<std::uint32_t>& ids);

private:
    struct ObjectState {
        bool        layer_set[kLayerCount] = {};
        std::uint32_t layer_rgb8[kLayerCount] = {};
        std::uint8_t opacity = 255;
    };

    void applyToViewport(const std::vector<std::uint32_t>& ids);
    void purgeModel(std::uint32_t session_model_id);

    bonsaiviewer::SessionState& session_;
    ViewportWindow& viewport_;
    std::unordered_map<std::uint32_t, ObjectState> states_;
};

} // namespace bonsaiviewer::services

#endif
