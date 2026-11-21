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

// A face of an object classified by its normal direction, in world space.
// `triangles` holds 9 floats per triangle (world); `area` is the summed area
// in m² and `centroid` the area-weighted centroid for the label.
// `contour` (M5) holds the 3D outline of the top surface: 6 floats per edge.
struct ClassifiedFace {
    double area = 0.0;
    float centroid[3] = {0, 0, 0};
    std::vector<float> triangles;
    std::vector<float> contour;    // 6 floats per edge (a, b), world
    std::vector<float> rejected;   // M5 dev: up-facing triangles outside the crest component (9 floats per tri)
};

// Result of the top/bottom-face detection on one object (world space, metres).
using TopFace = ClassifiedFace;

// World-space triangles of one object: 9 floats per triangle (3 vertices ×
// xyz), plus the object's world z range. Returns false if the object does
// not resolve to a live instance or has no mesh.
struct WorldMesh {
    std::vector<float> triangles;  // 9 floats per triangle, world space
    float zmin = 0.0f, zmax = 0.0f;
    // Mesh-local index list (3 per triangle, same order as `triangles`) and
    // per-vertex alpha byte (255 = opaque) — lets callers tell the glazed
    // (transparent) triangles of a window apart from its frame. Empty when
    // the CPU shadow carries no alpha.
    std::vector<uint32_t> indices;
    std::vector<uint8_t>  alpha;
};
WorldMesh worldTriangles(ViewportWindow& vp, std::uint32_t object_id);

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

// M5 — longueur_main_courante: developed ("déplié") length of the top
// surface of a railing / handrail element, in meters. The top surface is
// the connected component of upward-facing triangles (nz >= 0.5) that
// contains the highest triangle (the crest — e.g. the handrail on top of a
// guard rail, including inclined stair runs). The developed length is the
// mid-line of that strip: L = P/2 - 2A/P, with P the 3D perimeter of the
// surface outline and A its area (exact for a rectangular strip, e.g.
// 3.80 m landing + 4.73 m stair run). Falls back to the longest horizontal
// AABB dimension when no top surface is found.
// `surfaces_out` receives the detected top surface per object (triangles +
// 3D outline for display); `values_out` the per-object length.
// Summed over all object_ids.
double measureLongueurMainCourante(ViewportWindow& vp,
                                   const std::vector<std::uint32_t>& object_ids,
                                   std::vector<MeasuredObject>* objects_out = nullptr,
                                   std::vector<ClassifiedFace>* surfaces_out = nullptr,
                                   std::vector<double>* values_out = nullptr);

// M6 — surface_top: area (m²) of the top horizontal face of each object
// (triangles whose normal points up, nz >= 0.9, at the top of the object).
// `opening_threshold_m2` deducts openings smaller than the threshold are
// still counted; openings >= threshold are subtracted (approximated from
// the mesh: coplanar holes show up as missing triangles, so the deduction
// is a no-op on pure mesh — kept for the IFC-relation path).
// Returns the summed area; per-object top faces are appended to `faces_out`.
double measureSurfaceTop(ViewportWindow& vp,
                         const std::vector<std::uint32_t>& object_ids,
                         std::vector<MeasuredObject>* objects_out = nullptr,
                         std::vector<TopFace>* faces_out = nullptr);

// M7 — surface_bottom: area (m²) of the bottom horizontal face of each
// object (normal pointing down, nz <= -0.9, at the bottom of the object).
double measureSurfaceBottom(ViewportWindow& vp,
                            const std::vector<std::uint32_t>& object_ids,
                            std::vector<MeasuredObject>* objects_out = nullptr,
                            std::vector<TopFace>* faces_out = nullptr);

// M8 — surface_lateral: lateral surface area (m²) of each object, i.e. the
// total mesh area minus the top and bottom horizontal faces. `mode`:
//   0 = all lateral faces (walls), 1 = largest coplanar lateral face only,
//   2 = two largest coplanar lateral faces.
// Returns the summed area; per-object lateral faces are appended to `faces_out`.
double measureSurfaceLateral(ViewportWindow& vp,
                             const std::vector<std::uint32_t>& object_ids,
                             int mode,
                             std::vector<MeasuredObject>* objects_out = nullptr,
                             std::vector<ClassifiedFace>* faces_out = nullptr);

// M11 — surface_toiture_developpee: real (slope) area in m² of inclined
// roof surfaces — every triangle whose normal is neither up nor down
// (|nz| < 0.9). Summed over all object_ids.
double measureSurfaceToitureDeveloppee(ViewportWindow& vp,
                                       const std::vector<std::uint32_t>& object_ids,
                                       std::vector<MeasuredObject>* objects_out = nullptr,
                                       std::vector<ClassifiedFace>* faces_out = nullptr);

// M17 — volume_element: volume (m³) of each closed BREP, computed as the
// signed sum of tetrahedron volumes (divergence theorem over the mesh
// triangles, origin at world origin). Summed over all object_ids.
double measureVolumeElement(ViewportWindow& vp,
                            const std::vector<std::uint32_t>& object_ids,
                            std::vector<MeasuredObject>* objects_out = nullptr);

// M12 — surface_coffrage: formwork area (m²) of beams / columns / walls /
// slabs / footings. Faces are classified by their normal: "abouts" are the
// end faces perpendicular to the principal axis, the bottom face is the
// downward horizontal face, the top face the upward one, and the "joues"
// are the remaining lateral faces. By default only joues + bottom are
// counted; `include_abouts` and `include_top` add the end faces and the top
// face. Check formula for a beam: area = L × (2h + b).
double measureSurfaceCoffrage(ViewportWindow& vp,
                              const std::vector<std::uint32_t>& object_ids,
                              bool include_abouts,
                              bool include_top,
                              std::vector<MeasuredObject>* objects_out = nullptr,
                              std::vector<ClassifiedFace>* faces_out = nullptr);

// M13 — surface_section_projeté: developed area (m²) = section perimeter ×
// length, for rectangular duct segments. The principal axis is the longest
// AABB dimension; the section is the rectangle of the two remaining dims.
double measureSurfaceSectionProjetee(ViewportWindow& vp,
                                     const std::vector<std::uint32_t>& object_ids,
                                     std::vector<MeasuredObject>* objects_out = nullptr);

// M18 — volume_couche: layer volume (m³) = surface × thickness. The surface
// is the top horizontal face (M6) of each object; `thickness_m` is the
// layer thickness in metres (user parameter).
double measureVolumeCouche(ViewportWindow& vp,
                           const std::vector<std::uint32_t>& object_ids,
                           double thickness_m,
                           std::vector<MeasuredObject>* objects_out = nullptr,
                           std::vector<TopFace>* faces_out = nullptr);

// M20 — volume_entre: volume (m³) between the two selected objects.
// Approximation: for each pair (first half / second half of `object_ids`),
// volume = top-face area of the lower object × vertical gap to the bottom
// face of the upper object. Returns the summed volume over the pairs.
double measureVolumeEntre(ViewportWindow& vp,
                          const std::vector<std::uint32_t>& object_ids,
                          std::vector<MeasuredObject>* objects_out = nullptr);

// M27 — pente (surface): slope of each object's dominant face, in percent.
// Fits a plane z = ax + by + c over all mesh triangles (least squares on
// the area-weighted centroids); slope = sqrt(a² + b²) × 100, equivalent to
// tan(arccos(|nz|)) × 100 via the mean normal. `on_3d_length` divides by the
// 3D length instead of the horizontal one. Returns the mean slope (percent)
// over all object_ids.
double measurePente(ViewportWindow& vp,
                    const std::vector<std::uint32_t>& object_ids,
                    bool on_3d_length,
                    std::vector<MeasuredObject>* objects_out = nullptr);

// M21 — local metrics for one IfcSpace (geometry recomputed from its mesh):
// surface = bottom-face area (m²), périmètre = base perimeter (ml),
// hauteur = z extent (m), volume = surface × hauteur (m³, approximation).
struct LocalMetrics {
    double surface_m2 = 0.0;
    double perimetre_ml = 0.0;
    double hauteur_m = 0.0;
    double volume_m3 = 0.0;
    std::vector<float> floor_tris;  // bottom-face triangles, world
    float label_pos[3] = {0, 0, 0};
};
LocalMetrics measureLocalMetrics(ViewportWindow& vp, std::uint32_t object_id);

// M22 — local_surface_sous_plafond: floor area (m²) of an IfcSpace whose
// clear height is at least `min_height_m` (default 1.80 m). Returns the
// bottom-face area when hauteur >= min_height_m, else 0.
double measureLocalSurfaceSousPlafond(ViewportWindow& vp,
                                      std::uint32_t object_id,
                                      double min_height_m,
                                      std::vector<MeasuredObject>* objects_out = nullptr,
                                      std::vector<TopFace>* faces_out = nullptr);

} // namespace bonsaiviewer::modules::measurement

#endif // IFCINTERFACE_MODULES_MEASUREMENT_MEASUREMENTS_H
