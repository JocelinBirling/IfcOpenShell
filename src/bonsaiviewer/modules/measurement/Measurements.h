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

#ifndef IFCINTERFACE_MODULES_MEASUREMENT_MEASUREMENTS_H
#define IFCINTERFACE_MODULES_MEASUREMENT_MEASUREMENTS_H

#include <cstdint>
#include <vector>

class ViewportWindow;

namespace bonsaiviewer::modules::measurement {

// BonsaiViewer50 — Measurement functions M1-M5.
// All quantities are recomputed from the 3D geometry (AABB / mesh), never
// from IfcQuantity* (Qto) values stored in the IFC file.

// World AABB of one measured object, returned so the caller can draw the
// measured trait in the viewport.
struct MeasuredObject {
    std::uint32_t id = 0;
    float mn[3] = {0, 0, 0};
    float mx[3] = {0, 0, 0};
};

// Result of the top-face detection on one object (world space, metres).
struct TopFace {
    double area = 0.0;                // m², summed over the top coplanar triangles
    float centroid[3] = {0, 0, 0};    // area-weighted centroid, for the label
    std::vector<float> triangles;     // 9 floats per triangle (world), for highlighting
};

// M0 — aire_face_sup: detects the top face of `object_id` from its
// triangulation — triangles whose normal points up (nz >= 0.9) and whose
// height is at the very top of the object — and returns its area in m²
// together with the triangles themselves.
TopFace measureTopFace(ViewportWindow& vp, std::uint32_t object_id);

// M2 — longueur_axe: length along the element's principal axis, in meters.
// For linear elements (IfcRailing, IfcMember, IfcKerb, etc.) this is the
// longest AABB dimension; curves use the real 3D developed length.
// Summed over all object_ids.
double measureLongueurAxe(ViewportWindow& vp,
                          const std::vector<std::uint32_t>& object_ids,
                          std::vector<MeasuredObject>* objects_out = nullptr);

// M3 — perimetre: perimeter of the object's base footprint, in meters.
// Computed from the AABB's horizontal extent (2 * (dx + dy), Z is up).
// Summed over all object_ids.
double measurePerimetre(ViewportWindow& vp,
                        const std::vector<std::uint32_t>& object_ids,
                        std::vector<MeasuredObject>* objects_out = nullptr);

// M4 — longueur_arete: length of the longest extracted edge, in meters.
// Currently approximated by the longest AABB edge; a full implementation
// would extract the actual edge from face intersections.
// Summed over all object_ids.
double measureLongueurArete(ViewportWindow& vp,
                            const std::vector<std::uint32_t>& object_ids,
                            std::vector<MeasuredObject>* objects_out = nullptr);

// M5 — longueur_main_courante: developed length of the top coplanar face
// of a railing / handrail element, in meters. Uses the longest horizontal
// AABB dimension of the top face.
// Summed over all object_ids.
double measureLongueurMainCourante(ViewportWindow& vp,
                                   const std::vector<std::uint32_t>& object_ids,
                                   std::vector<MeasuredObject>* objects_out = nullptr);

} // namespace bonsaiviewer::modules::measurement

#endif // IFCINTERFACE_MODULES_MEASUREMENT_MEASUREMENTS_H
