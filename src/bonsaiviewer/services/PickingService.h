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

#ifndef IFCINTERFACE_SERVICES_PICKINGSERVICE_H
#define IFCINTERFACE_SERVICES_PICKINGSERVICE_H

#include <QObject>

#include <cstdint>
#include <vector>

class ViewportWindow;

// B09 — PickingService (see 00_Technical_Implementation/
// BonsaiViewerTI00). Shell-side facade over the viewport's raycast / pick
// passes. Entity-level results: hit object id, hit type (point / edge /
// face), world position, surface normal. The viewport's refined pick
// resolves a triangle-accurate world position + normal; vertex / edge
// classification still needs hit-triangle connectivity from the render
// engine, so pickAt currently reports Face for every entity hit — TI11's
// hover feedback extends this when the engine exposes it. Hover is off by
// default (a pick renders a one-shot pass, too expensive to run per mouse
// move uninvited); features enable it and react to the signals. All pixel
// coordinates are physical (post-DPR), matching ViewportWindow's pick API.

namespace bonsaiviewer::services {

class PickingService : public QObject {
    Q_OBJECT

public:
    enum class HitType {
        None,
        Point,
        Edge,
        Face,
    };

    struct PickResult {
        std::uint32_t object_id = 0;
        std::uint32_t session_model_id = 0;
        std::uint32_t mesh_id = 0;
        HitType hit_type = HitType::None;
        float world_pos[3] = {0.0f, 0.0f, 0.0f};
        float world_normal[3] = {0.0f, 0.0f, 0.0f};
        // Distance from the camera eye to the hit point (world units).
        float distance = 0.0f;
    };

    explicit PickingService(ViewportWindow& viewport, QObject* parent = nullptr);

    // Pick at a pixel: triangle-accurate world position + camera-facing
    // normal from the viewport's refined pick. False on a background miss.
    bool pickAt(int x_px, int y_px, PickResult& out);

    // World-space raycast against the CPU mesh shadows. `dir` must be a
    // unit vector; `out.distance` is the ray's t parameter.
    bool raycastAt(const float origin[3], const float dir[3], PickResult& out);

    // Every unique object id covered by a physical-pixel rectangle.
    std::vector<std::uint32_t> pickRect(int x, int y, int w, int h);

    void setHoverEnabled(bool enabled);

signals:
    // Hover object under the cursor (throttled), 0 / hoverCleared when the
    // cursor leaves the model.
    void objectHovered(std::uint32_t object_id);
    void hoverCleared();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    ViewportWindow& viewport_;
    bool hover_enabled_ = false;
    std::uint32_t last_hover_id_ = 0;
};

} // namespace bonsaiviewer::services

#endif
