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
// multiplies it. The rasterised PDF page (PdfRender) is drawn on the quad
// through `corners` + `uvs`; when no rasterizer is available the caller
// falls back to a translucent fill of `quad_triangles`.

struct PlanGeometry {
    std::vector<float> quad_triangles; // 2 triangles, 9 floats each (world xyz)
    float corners[4][3] = {{0.f, 0.f, 0.f}}; // world xyz, plan order (-,-) (+,-) (+,+) (-,+)
    // Texture coords matching `corners`: the image's top row is the plan's
    // +Y edge, so the bottom corners carry v = 1.
    float uvs[4][2] = {{0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f}, {0.f, 0.f}};
    float center[3] = {0.f, 0.f, 0.f}; // for the overlay label
    double width_m = 0.0;
    double height_m = 0.0;
};

// 1 PDF point in meters.
inline constexpr double kPointToMeter = 25.4 / 72000.0;

// The native size (meters) of the plan source, from the PDF MediaBox of the
// given page (0-based) when readable; fallback `fallback` otherwise.
// NOTE: PDFs with compressed page objects (object streams) have no
// plain-text MediaBox — use PdfRender::pdfPageSizePoints for those.
void nativeSize(const QString& path, int page, double& width_m, double& height_m,
                double fallback_w = 10.0, double fallback_h = 10.0);

// The world-space quad of a plan with the given configuration. The native
// size comes from nativeSize() (PDF MediaBox, else the fallback).
PlanGeometry planQuad(const QString& source_path, const PlanConfig& config);
// Same, with an explicit native size in meters (e.g. from
// PdfRender::pdfPageSizePoints for compressed PDFs).
PlanGeometry planQuad(const QString& source_path, const PlanConfig& config,
                      double native_w_m, double native_h_m);

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PLANOVERLAY_H
