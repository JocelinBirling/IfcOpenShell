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

#include "PickingService.h"

#include "../ifcviewer/CameraMath.h"
#include "../ifcviewer/ViewportWindow.h"

#include <QEvent>
#include <QMouseEvent>
#include <QTimer>

#include <cmath>

namespace bonsaiviewer::services {

namespace {

// Mirrors ViewportCore::orbitEye: world +Z up, yaw about Z, pitch above
// the XY plane. Kept local so the service only needs the public cameraState.
Eigen::Vector3f orbitEye(const ViewportWindow& viewport) {
    const auto camera = viewport.cameraState();
    constexpr float kDeg2Rad = kPiF / 180.0f;
    const float yaw = camera.yaw * kDeg2Rad;
    const float pit = camera.pitch * kDeg2Rad;
    const float cp = std::cos(pit), sp = std::sin(pit);
    const float cy = std::cos(yaw), sy = std::sin(yaw);
    return Eigen::Vector3f(camera.target.x() + camera.distance * cp * cy,
                           camera.target.y() + camera.distance * cp * sy,
                           camera.target.z() + camera.distance * sp);
}

constexpr int kHoverThrottleMs = 30;

} // namespace

PickingService::PickingService(ViewportWindow& viewport, QObject* parent)
    : QObject(parent), viewport_(viewport) {}

bool PickingService::pickAt(int x_px, int y_px, PickResult& out) {
    ViewportWindow::MeshLocalPick pick;
    if (!viewport_.pickMeshLocalAt(x_px, y_px, pick)) {
        out = PickResult{};
        return false;
    }
    out = PickResult{};
    out.object_id = pick.object_id;
    out.session_model_id = pick.session_model_id;
    out.mesh_id = pick.mesh_id;
    out.hit_type = HitType::Face;
    for (int i = 0; i < 3; ++i) {
        out.world_pos[i] = pick.world_pos[i];
        out.world_normal[i] = pick.world_normal[i];
    }
    const Eigen::Vector3f pos(pick.world_pos[0], pick.world_pos[1], pick.world_pos[2]);
    out.distance = (pos - orbitEye(viewport_)).norm();
    return true;
}

bool PickingService::raycastAt(const float origin[3], const float dir[3],
                               PickResult& out) {
    ViewportWindow::RaycastHit hit;
    if (!viewport_.raycast(origin, dir, hit)) {
        out = PickResult{};
        return false;
    }
    out = PickResult{};
    out.object_id = hit.object_id;
    out.hit_type = HitType::Face;
    out.distance = hit.distance;
    for (int i = 0; i < 3; ++i) {
        out.world_pos[i] = hit.world_pos[i];
        out.world_normal[i] = hit.world_normal[i];
    }
    return true;
}

std::vector<std::uint32_t> PickingService::pickRect(int x, int y, int w, int h) {
    return viewport_.picksInRect(x, y, w, h);
}

void PickingService::setHoverEnabled(bool enabled) {
    if (hover_enabled_ == enabled) return;
    hover_enabled_ = enabled;
    if (enabled) {
        viewport_.installEventFilter(this);
    } else {
        viewport_.removeEventFilter(this);
        if (last_hover_id_ != 0) {
            last_hover_id_ = 0;
            emit hoverCleared();
        }
    }
}

bool PickingService::eventFilter(QObject* watched, QEvent* event) {
    if (watched == &viewport_ && event->type() == QEvent::MouseMove) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        // A hover pick encodes a one-shot render pass; coalesce moves into
        // a single pick per throttle window on the timer's timeout, at the
        // last cursor position (physical pixels, post-DPR).
        const float dpr = viewport_.dpr();
        const int x_px = int(mouse->position().x() * dpr);
        const int y_px = int(mouse->position().y() * dpr);
        QTimer::singleShot(kHoverThrottleMs, Qt::CoarseTimer, this, [this, x_px, y_px]() {
            if (!hover_enabled_) return;
            const std::uint32_t id = viewport_.pickObjectAt(x_px, y_px);
            if (id == last_hover_id_) return;
            if (id != 0) emit objectHovered(id);
            else emit hoverCleared();
            last_hover_id_ = id;
        });
    }
    return QObject::eventFilter(watched, event);
}

} // namespace bonsaiviewer::services
