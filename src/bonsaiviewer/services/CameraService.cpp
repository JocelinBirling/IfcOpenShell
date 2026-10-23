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

#include "CameraService.h"

#include "../ElementRegistry.h"
#include "../SessionState.h"
#include "../ifcviewer/CameraMath.h"
#include "../ifcviewer/ViewportWindow.h"
#include "PopupService.h"

#include <QWidget>

#include <cmath>
#include <limits>

namespace bonsaiviewer::services {

namespace {

constexpr float kFramePadding = 1.10f;       // matches ViewportCore::viewAll
constexpr float kNearPlaneMeters = 200.0f;   // TI02 fit-view heuristic
constexpr float kZoomStep = 0.9f;            // distance factor per wheel step

Eigen::Vector3f orbitEyeOf(const ViewportWindow& viewport) {
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

} // namespace

CameraService::CameraService(bonsaiviewer::SessionState& session,
                             ViewportWindow& viewport,
                             QObject* parent)
    : QObject(parent), session_(session), viewport_(viewport) {}

void CameraService::fitView() {
    Eigen::Vector3f mn(std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::infinity());
    Eigen::Vector3f mx(-std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity());
    bool any_visible = false;
    for (std::uint32_t session_model_id : session_.sessionModelIds()) {
        for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
            if (viewport_.isObjectHidden(info.object_id)) continue;
            Eigen::Vector3f o_mn, o_mx;
            if (!viewport_.computeObjectAabb(info.object_id, o_mn, o_mx)) continue;
            any_visible = true;
            mn = mn.cwiseMin(o_mn);
            mx = mx.cwiseMax(o_mx);
        }
    }
    if (!any_visible) {
        popup::information(nullptr, QStringLiteral("The active view is empty."));
        return;
    }
    viewport_.frameAabb(mn.data(), mx.data(), kFramePadding);

    // 200 m heuristic: after framing, an object centre must sit within
    // 200 m of the view plane, else the scene is effectively out of frame.
    const auto camera = viewport_.cameraState();
    const Eigen::Vector3f target = camera.target;
    const Eigen::Vector3f forward = (target - cameraEye()).normalized();
    bool near_plane = false;
    for (std::uint32_t session_model_id : session_.sessionModelIds()) {
        for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
            if (viewport_.isObjectHidden(info.object_id)) continue;
            Eigen::Vector3f o_mn, o_mx;
            if (!viewport_.computeObjectAabb(info.object_id, o_mn, o_mx)) continue;
            const Eigen::Vector3f centre = 0.5f * (o_mn + o_mx);
            if (std::abs((centre - target).dot(forward)) <= kNearPlaneMeters) {
                near_plane = true;
                break;
            }
        }
        if (near_plane) break;
    }
    if (!near_plane) {
        popup::information(nullptr, QStringLiteral("The objects are far away."));
    }
}

void CameraService::framePerpendicular(const Eigen::Vector3f& world_normal) {
    // View direction is against the normal: forward = -normal. From
    // orbitEye, forward = -(cp*cy, cp*sy, sp) with cp >= 0, so
    // sp = -forward.z and yaw = atan2(-forward.y, -forward.x).
    const Eigen::Vector3f forward = -world_normal.normalized();
    const float pitch = std::asin(std::clamp(-forward.z(), -1.0f, 1.0f)) * 180.0f / kPiF;
    const float yaw = std::atan2(-forward.y(), -forward.x()) * 180.0f / kPiF;
    const auto camera = viewport_.cameraState();
    // Keep just shy of the pole so the orbit up-vector switch does not
    // degenerate the view (matches the viewport's own clamp).
    const float clamped_pitch = std::clamp(pitch, -89.0f, 89.0f);
    viewport_.setCamera(camera.target.x(), camera.target.y(), camera.target.z(),
                        camera.distance, yaw, clamped_pitch);
}

void CameraService::zoomCenter(int steps) {
    zoomToward(viewport_.cameraState().target, steps);
}

void CameraService::zoomAtCursor(int x_px, int y_px, int steps) {
    std::uint32_t object_id = 0;
    Eigen::Vector3f world_pos, world_normal;
    if (!viewport_.pickSurfaceAt(x_px, y_px, object_id, world_pos, world_normal)) {
        zoomCenter(steps);
        return;
    }
    zoomToward(world_pos, steps);
}

void CameraService::zoomOnSelection(int steps) {
    const auto& selection = viewport_.selection().selectionIds();
    if (selection.empty()) {
        zoomCenter(steps);
        return;
    }
    Eigen::Vector3f mn(std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::infinity());
    Eigen::Vector3f mx(-std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity());
    bool any = false;
    for (std::uint32_t id : selection) {
        Eigen::Vector3f o_mn, o_mx;
        if (!viewport_.computeObjectAabb(id, o_mn, o_mx)) continue;
        any = true;
        mn = mn.cwiseMin(o_mn);
        mx = mx.cwiseMax(o_mx);
    }
    if (!any) {
        zoomCenter(steps);
        return;
    }
    zoomToward(0.5f * (mn + mx), steps);
}

void CameraService::setOrbitPivotAtCursor(int x_px, int y_px) {
    std::uint32_t object_id = 0;
    Eigen::Vector3f world_pos, world_normal;
    if (!viewport_.pickSurfaceAt(x_px, y_px, object_id, world_pos, world_normal)) {
        return;
    }
    const auto camera = viewport_.cameraState();
    viewport_.setCamera(world_pos.x(), world_pos.y(), world_pos.z(),
                        camera.distance, camera.yaw, camera.pitch);
}

Eigen::Vector3f CameraService::cameraEye() const {
    return orbitEyeOf(viewport_);
}

void CameraService::zoomToward(const Eigen::Vector3f& anchor, int steps) {
    // Scale the distance and slide the target along (target - anchor) so
    // the anchor keeps its approximate screen position through the zoom.
    const float factor = std::pow(kZoomStep, float(steps));
    const auto camera = viewport_.cameraState();
    const Eigen::Vector3f target = camera.target;
    const Eigen::Vector3f new_target = anchor + (target - anchor) * factor;
    viewport_.setCamera(new_target.x(), new_target.y(), new_target.z(),
                        std::max(0.1f, camera.distance * factor),
                        camera.yaw, camera.pitch);
}

} // namespace bonsaiviewer::services
