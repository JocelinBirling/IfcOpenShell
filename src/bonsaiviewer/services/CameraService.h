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

#ifndef IFCINTERFACE_SERVICES_CAMERASERVICE_H
#define IFCINTERFACE_SERVICES_CAMERASERVICE_H

#include <QObject>

#include <Eigen/Dense>

#include <cstdint>
#include <vector>

class ViewportWindow;

namespace bonsaiviewer { class SessionState; }

// B10 — CameraService (see 00_Technical_Implementation/
// BonsaiViewerTI00). Fit view on the visible elements with the 200 m
// heuristic and its popups (TI02), perpendicular-to-normal framing, and
// the zoom / orbit preset behaviours of TI08: one wheel step is routed to
// zoomCenter / zoomAtCursor / zoomOnSelection, and the orbit pivot is
// fixed at the cursor at drag start. Pixel coordinates are physical
// (post-DPR).

namespace bonsaiviewer::services {

class CameraService : public QObject {
    Q_OBJECT

public:
    CameraService(bonsaiviewer::SessionState& session,
                  ViewportWindow& viewport,
                  QObject* parent = nullptr);

    // Frame the union AABB of the visible elements, then look for one
    // object within 200 m of the view plane ("The objects are far away"
    // popup when not, "The active view is empty" when nothing is visible).
    void fitView();

    // Aim the view plane perpendicular to `world_normal`, facing against
    // it, keeping the current target and distance (TI02 Perpendicular
    // View).
    void framePerpendicular(const Eigen::Vector3f& world_normal);

    // One wheel step: positive zooms in, negative zooms out. The cursor
    // variants fall back to the centre behaviour on a background miss.
    void zoomCenter(int steps);
    void zoomAtCursor(int x_px, int y_px, int steps);
    void zoomOnSelection(int steps);

    // Orbit preset: move the orbit pivot onto the model under the cursor
    // (view-centre orbit is the default behaviour and needs no call).
    void setOrbitPivotAtCursor(int x_px, int y_px);

private:
    Eigen::Vector3f cameraEye() const;
    void zoomToward(const Eigen::Vector3f& anchor, int steps);

    bonsaiviewer::SessionState& session_;
    ViewportWindow& viewport_;
};

} // namespace bonsaiviewer::services

#endif
