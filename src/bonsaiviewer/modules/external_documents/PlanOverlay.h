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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PLANOVERLAY_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PLANOVERLAY_H

#include "Types.h"

#include <QString>
#include <vector>

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer36 — the geometry of a 2D plan as a background of the 3D
// view: a flat quad at its elevation, with its own rotation, scale and
// translation.
//
// The plan's native size comes from the PDF page MediaBox when the source is
// a .pdf (1 pt = 25.4/72000 m), else from a default drawing size; `scale`
// multiplies it. Phase 1 rasterises no PDF content: the quad is drawn as a
// translucent overlay (ViewportWindow::setHighlightTriangles) with a label
// at its centre — the PDF rendering library is an open subject of the spec.

struct PlanGeometry {
    std::vector<float> quad_triangles; // 2 triangles, 9 floats each (world xyz)
    float center[3] = {0.f, 0.f, 0.f}; // for the overlay label
    double width_m = 0.0;
    double height_m = 0.0;
};

// The native size (meters) of the plan source, from the PDF MediaBox of the
// given page (0-based) when readable; fallback `fallback` otherwise.
void nativeSize(const QString& path, int page, double& width_m, double& height_m,
                double fallback_w = 10.0, double fallback_h = 10.0);

// The world-space quad of a plan with the given configuration.
PlanGeometry planQuad(const QString& source_path, const PlanConfig& config);

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PLANOVERLAY_H
