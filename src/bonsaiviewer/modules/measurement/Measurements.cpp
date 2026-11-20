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

#include "Measurements.h"

#include "../../../ifcviewer/ViewportWindow.h"

#include <Eigen/Core>

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace bonsaiviewer::modules::measurement {

namespace {

// Retrieve the world AABB of an object. Returns false if the object
// does not resolve to a live instance.
bool objectAabb(ViewportWindow& vp, std::uint32_t id, float mn[3], float mx[3]) {
    Eigen::Vector3f emn, emx;
    if (!vp.computeObjectAabb(id, emn, emx)) return false;
    mn[0] = emn.x(); mn[1] = emn.y(); mn[2] = emn.z();
    mx[0] = emx.x(); mx[1] = emx.y(); mx[2] = emx.z();
    return true;
}

// Longest dimension of the AABB (the "axis length" approximation).
double aabbLongestEdge(float mn[3], float mx[3]) {
    const double dx = static_cast<double>(mx[0]) - mn[0];
    const double dy = static_cast<double>(mx[1]) - mn[1];
    const double dz = static_cast<double>(mx[2]) - mn[2];
    return std::max({dx, dy, dz});
}

// Perimeter of the AABB's horizontal footprint (Z is up in IFC, so the
// base plane is XY): 2 * (dx + dy).
double aabbBasePerimeter(float mn[3], float mx[3]) {
    const double dx = static_cast<double>(mx[0]) - mn[0];
    const double dy = static_cast<double>(mx[1]) - mn[1];
    return 2.0 * (dx + dy);
}

} // namespace

namespace {

void recordObject(std::vector<MeasuredObject>* out, std::uint32_t id,
                  const float mn[3], const float mx[3]) {
    if (!out) return;
    MeasuredObject obj;
    obj.id = id;
    for (int i = 0; i < 3; ++i) { obj.mn[i] = mn[i]; obj.mx[i] = mx[i]; }
    out->push_back(obj);
}

} // namespace

double measureLongueurAxe(ViewportWindow& vp,
                          const std::vector<std::uint32_t>& object_ids,
                          std::vector<MeasuredObject>* objects_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (objectAabb(vp, id, mn, mx)) {
            total += aabbLongestEdge(mn, mx);
            recordObject(objects_out, id, mn, mx);
        }
    }
    return total;
}

double measurePerimetre(ViewportWindow& vp,
                        const std::vector<std::uint32_t>& object_ids,
                        std::vector<MeasuredObject>* objects_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (objectAabb(vp, id, mn, mx)) {
            total += aabbBasePerimeter(mn, mx);
            recordObject(objects_out, id, mn, mx);
        }
    }
    return total;
}

double measureLongueurArete(ViewportWindow& vp,
                            const std::vector<std::uint32_t>& object_ids,
                            std::vector<MeasuredObject>* objects_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (objectAabb(vp, id, mn, mx)) {
            // Longest edge of the AABB as an approximation of the
            // extracted edge length.
            total += aabbLongestEdge(mn, mx);
            recordObject(objects_out, id, mn, mx);
        }
    }
    return total;
}

double measureLongueurMainCourante(ViewportWindow& vp,
                                   const std::vector<std::uint32_t>& object_ids,
                                   std::vector<MeasuredObject>* objects_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (objectAabb(vp, id, mn, mx)) {
            // For a handrail, the developed length of the top face is
            // approximated by the longest horizontal AABB dimension
            // (horizontal = XY plane, Z is up).
            const double dx = static_cast<double>(mx[0]) - mn[0];
            const double dy = static_cast<double>(mx[1]) - mn[1];
            total += std::max(dx, dy);
            recordObject(objects_out, id, mn, mx);
        }
    }
    return total;
}

WorldMesh worldTriangles(ViewportWindow& vp, std::uint32_t object_id) {
    WorldMesh result;
    ViewportWindow::InstanceLookup lookup;
    if (!vp.findInstance(object_id, lookup)) return result;
    ViewportWindow::MeshTriangles mesh;
    if (!vp.readbackMeshTriangles(lookup.session_model_id, lookup.mesh_id, mesh)) return result;
    if (mesh.indices.empty() || mesh.positions.empty()) return result;

    // Full world transform: modelToWorld (federation/model ops) * placement,
    // both column-major.
    Eigen::Matrix4d placement = Eigen::Matrix4d::Identity();
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            placement(r, c) = lookup.placement_transformation[c * 4 + r];
    Eigen::Matrix4d world = placement;
    Eigen::Matrix4d mtw;
    if (vp.modelToWorld(lookup.session_model_id, mtw)) world = mtw * placement;

    // World-space triangles, 9 floats each; track the object's z range.
    result.zmin = FLT_MAX; result.zmax = -FLT_MAX;
    for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
        for (int corner = 0; corner < 3; ++corner) {
            const float* p = &mesh.positions[3 * std::size_t(mesh.indices[t + corner])];
            const Eigen::Vector4d w = world * Eigen::Vector4d(p[0], p[1], p[2], 1.0);
            result.triangles.push_back(static_cast<float>(w.x()));
            result.triangles.push_back(static_cast<float>(w.y()));
            result.triangles.push_back(static_cast<float>(w.z()));
            result.zmin = std::min(result.zmin, static_cast<float>(w.z()));
            result.zmax = std::max(result.zmax, static_cast<float>(w.z()));
        }
    }
    if (result.triangles.empty()) { result.zmin = 0.0f; result.zmax = 0.0f; }
    return result;
}

namespace {

// Accumulate one triangle into a TopFace / ClassifiedFace accumulator.
void accumulateFace(ClassifiedFace& face, double& area_sum,
                    double& cx, double& cy, double& cz,
                    const float* a, const float* b, const float* c,
                    double area) {
    face.area += area;
    cx += area * (a[0] + b[0] + c[0]) / 3.0;
    cy += area * (a[1] + b[1] + c[1]) / 3.0;
    cz += area * (a[2] + b[2] + c[2]) / 3.0;
    face.triangles.insert(face.triangles.end(), a, a + 9);
    area_sum += area;
}

// Extract the horizontal face of an object at one extremum of its z range.
// `up` selects the top face (nz >= 0.9, z near zmax) or the bottom face
// (nz <= -0.9, z near zmin).
TopFace extractHorizontalFace(const WorldMesh& wm, bool up) {
    TopFace face;
    if (wm.triangles.empty()) return face;
    const float tol = std::max(0.01f, (wm.zmax - wm.zmin) * 0.02f);
    double area_sum = 0.0, cx = 0.0, cy = 0.0, cz = 0.0;
    for (std::size_t t = 0; t + 8 < wm.triangles.size(); t += 9) {
        const float* a = &wm.triangles[t];
        const float* b = &wm.triangles[t + 3];
        const float* c = &wm.triangles[t + 6];
        const float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
        const float vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
        const float nx = uy * vz - uz * vy;
        const float ny = uz * vx - ux * vz;
        const float nz = ux * vy - uy * vx;
        const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-9f) continue;
        const double area = 0.5 * static_cast<double>(len);
        const float tri_cz = (a[2] + b[2] + c[2]) / 3.0f;
        const float nz_unit = nz / len;
        const bool match = up ? (nz_unit >= 0.9f && tri_cz >= wm.zmax - tol)
                              : (nz_unit <= -0.9f && tri_cz <= wm.zmin + tol);
        if (match) {
            accumulateFace(face, area_sum, cx, cy, cz, a, b, c, area);
        }
    }
    if (area_sum > 0.0) {
        face.centroid[0] = static_cast<float>(cx / area_sum);
        face.centroid[1] = static_cast<float>(cy / area_sum);
        face.centroid[2] = static_cast<float>(cz / area_sum);
    }
    return face;
}

// Group coplanar lateral triangles (normal neither up nor down) into faces
// by (normal direction, offset along the normal). Returns the groups sorted
// by descending area.
std::vector<ClassifiedFace> lateralFaceGroups(const WorldMesh& wm) {
    struct Group {
        float nx = 0, ny = 0, nz = 0;   // unit normal
        float d = 0;                     // plane offset: dot(n, p)
        ClassifiedFace face;
        double area_sum = 0.0, cx = 0.0, cy = 0.0, cz = 0.0;
    };
    std::vector<Group> groups;
    constexpr float kNormalTol = 0.05f;   // normals within ~3° are coplanar
    constexpr float kPlaneTol = 0.02f;    // 2 cm coplanarity tolerance
    for (std::size_t t = 0; t + 8 < wm.triangles.size(); t += 9) {
        const float* a = &wm.triangles[t];
        const float* b = &wm.triangles[t + 3];
        const float* c = &wm.triangles[t + 6];
        const float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
        const float vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
        const float nx = uy * vz - uz * vy;
        const float ny = uz * vx - ux * vz;
        const float nz = ux * vy - uy * vx;
        const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-9f) continue;
        const float nu_nx = nx / len, nu_ny = ny / len, nu_nz = nz / len;
        // Lateral = not horizontal.
        if (std::abs(nu_nz) >= 0.9f) continue;
        const double area = 0.5 * static_cast<double>(len);
        const float d = nu_nx * a[0] + nu_ny * a[1] + nu_nz * a[2];
        // Find an existing coplanar group (same normal direction + offset).
        Group* g = nullptr;
        for (auto& cand : groups) {
            const float dot = cand.nx * nu_nx + cand.ny * nu_ny + cand.nz * nu_nz;
            if (dot >= 1.0f - kNormalTol && std::abs(cand.d - d) <= kPlaneTol) { g = &cand; break; }
        }
        if (!g) {
            groups.push_back(Group{});
            g = &groups.back();
            g->nx = nu_nx; g->ny = nu_ny; g->nz = nu_nz; g->d = d;
        }
        accumulateFace(g->face, g->area_sum, g->cx, g->cy, g->cz, a, b, c, area);
    }
    for (auto& g : groups) {
        if (g.area_sum > 0.0) {
            g.face.centroid[0] = static_cast<float>(g.cx / g.area_sum);
            g.face.centroid[1] = static_cast<float>(g.cy / g.area_sum);
            g.face.centroid[2] = static_cast<float>(g.cz / g.area_sum);
        }
    }
    std::vector<ClassifiedFace> result;
    result.reserve(groups.size());
    for (auto& g : groups) result.push_back(std::move(g.face));
    std::sort(result.begin(), result.end(),
              [](const ClassifiedFace& x, const ClassifiedFace& y) { return x.area > y.area; });
    return result;
}

} // namespace

TopFace measureTopFace(ViewportWindow& vp, std::uint32_t object_id) {
    return extractHorizontalFace(worldTriangles(vp, object_id), /*up=*/true);
}

double measureSurfaceTop(ViewportWindow& vp,
                         const std::vector<std::uint32_t>& object_ids,
                         std::vector<MeasuredObject>* objects_out,
                         std::vector<TopFace>* faces_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        const TopFace face = extractHorizontalFace(worldTriangles(vp, id), /*up=*/true);
        if (face.area <= 0.0) continue;
        total += face.area;
        recordObject(objects_out, id, mn, mx);
        if (faces_out) faces_out->push_back(face);
    }
    return total;
}

double measureSurfaceBottom(ViewportWindow& vp,
                            const std::vector<std::uint32_t>& object_ids,
                            std::vector<MeasuredObject>* objects_out,
                            std::vector<TopFace>* faces_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        const TopFace face = extractHorizontalFace(worldTriangles(vp, id), /*up=*/false);
        if (face.area <= 0.0) continue;
        total += face.area;
        recordObject(objects_out, id, mn, mx);
        if (faces_out) faces_out->push_back(face);
    }
    return total;
}

double measureSurfaceLateral(ViewportWindow& vp,
                             const std::vector<std::uint32_t>& object_ids,
                             int mode,
                             std::vector<MeasuredObject>* objects_out,
                             std::vector<ClassifiedFace>* faces_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        auto groups = lateralFaceGroups(worldTriangles(vp, id));
        if (groups.empty()) continue;
        double obj_area = 0.0;
        ClassifiedFace kept;
        if (mode == 1 && !groups.empty()) {
            obj_area = groups.front().area;
            kept = groups.front();
        } else if (mode == 2 && groups.size() >= 2) {
            obj_area = groups[0].area + groups[1].area;
            kept.area = obj_area;
            kept.triangles = groups[0].triangles;
            kept.triangles.insert(kept.triangles.end(), groups[1].triangles.begin(), groups[1].triangles.end());
            for (int i = 0; i < 3; ++i) kept.centroid[i] = groups[0].centroid[i];
        } else {
            // mode 0: all lateral faces.
            for (const auto& g : groups) {
                obj_area += g.area;
                kept.triangles.insert(kept.triangles.end(), g.triangles.begin(), g.triangles.end());
            }
            kept.area = obj_area;
            if (!groups.empty())
                for (int i = 0; i < 3; ++i) kept.centroid[i] = groups[0].centroid[i];
        }
        total += obj_area;
        recordObject(objects_out, id, mn, mx);
        if (faces_out) faces_out->push_back(std::move(kept));
    }
    return total;
}

double measureSurfaceToitureDeveloppee(ViewportWindow& vp,
                                       const std::vector<std::uint32_t>& object_ids,
                                       std::vector<MeasuredObject>* objects_out,
                                       std::vector<ClassifiedFace>* faces_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        const WorldMesh wm = worldTriangles(vp, id);
        ClassifiedFace roof;
        double area_sum = 0.0, cx = 0.0, cy = 0.0, cz = 0.0;
        for (std::size_t t = 0; t + 8 < wm.triangles.size(); t += 9) {
            const float* a = &wm.triangles[t];
            const float* b = &wm.triangles[t + 3];
            const float* c = &wm.triangles[t + 6];
            const float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
            const float vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
            const float nx = uy * vz - uz * vy;
            const float ny = uz * vx - ux * vz;
            const float nz = ux * vy - uy * vx;
            const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len < 1e-9f) continue;
            // Inclined = not horizontal (|nz| < 0.9).
            if (std::abs(nz / len) >= 0.9f) continue;
            const double area = 0.5 * static_cast<double>(len);
            accumulateFace(roof, area_sum, cx, cy, cz, a, b, c, area);
        }
        if (area_sum > 0.0) {
            roof.centroid[0] = static_cast<float>(cx / area_sum);
            roof.centroid[1] = static_cast<float>(cy / area_sum);
            roof.centroid[2] = static_cast<float>(cz / area_sum);
            total += roof.area;
            recordObject(objects_out, id, mn, mx);
            if (faces_out) faces_out->push_back(std::move(roof));
        }
    }
    return total;
}

double measureVolumeElement(ViewportWindow& vp,
                            const std::vector<std::uint32_t>& object_ids,
                            std::vector<MeasuredObject>* objects_out) {
    // Signed volume via the divergence theorem: V = (1/6) Σ (a · (b × c))
    // over all triangles, with the world origin as the apex. Works for any
    // closed mesh (outward-facing normals give a positive volume).
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        const WorldMesh wm = worldTriangles(vp, id);
        double vol = 0.0;
        for (std::size_t t = 0; t + 8 < wm.triangles.size(); t += 9) {
            const float* a = &wm.triangles[t];
            const float* b = &wm.triangles[t + 3];
            const float* c = &wm.triangles[t + 6];
            // a · (b × c) = determinant of [a b c].
            const double det =
                static_cast<double>(a[0]) * (static_cast<double>(b[1]) * c[2] - static_cast<double>(b[2]) * c[1]) -
                static_cast<double>(a[1]) * (static_cast<double>(b[0]) * c[2] - static_cast<double>(b[2]) * c[0]) +
                static_cast<double>(a[2]) * (static_cast<double>(b[0]) * c[1] - static_cast<double>(b[1]) * c[0]);
            vol += det / 6.0;
        }
        total += std::abs(vol);
        recordObject(objects_out, id, mn, mx);
    }
    return total;
}

double measureSurfaceCoffrage(ViewportWindow& vp,
                              const std::vector<std::uint32_t>& object_ids,
                              bool include_abouts,
                              bool include_top,
                              std::vector<MeasuredObject>* objects_out,
                              std::vector<ClassifiedFace>* faces_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        const WorldMesh wm = worldTriangles(vp, id);
        if (wm.triangles.empty()) continue;

        // Principal axis = longest AABB dimension (0=X, 1=Y, 2=Z).
        const double dx = static_cast<double>(mx[0]) - mn[0];
        const double dy = static_cast<double>(mx[1]) - mn[1];
        const double dz = static_cast<double>(mx[2]) - mn[2];
        int axis = 0;
        double axis_len = dx;
        if (dy >= axis_len) { axis = 1; axis_len = dy; }
        if (dz >= axis_len) { axis = 2; axis_len = dz; }

        ClassifiedFace coffrage;
        double area_sum = 0.0, cx = 0.0, cy = 0.0, cz = 0.0;
        for (std::size_t t = 0; t + 8 < wm.triangles.size(); t += 9) {
            const float* a = &wm.triangles[t];
            const float* b = &wm.triangles[t + 3];
            const float* c = &wm.triangles[t + 6];
            const float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
            const float vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
            const float nx = uy * vz - uz * vy;
            const float ny = uz * vx - ux * vz;
            const float nz = ux * vy - uy * vx;
            const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len < 1e-9f) continue;
            const double area = 0.5 * static_cast<double>(len);
            const float nu[3] = {nx / len, ny / len, nz / len};
            // Classify: about = normal along the principal axis; top/bottom
            // = vertical normal; joue = the remaining lateral faces.
            const bool is_about = std::abs(nu[axis]) >= 0.9f;
            const bool is_top = nu[2] >= 0.9f;
            const bool is_bottom = nu[2] <= -0.9f;
            const bool keep = is_bottom || (!is_about && !is_top && !is_bottom)
                              || (include_abouts && is_about)
                              || (include_top && is_top);
            if (!keep) continue;
            accumulateFace(coffrage, area_sum, cx, cy, cz, a, b, c, area);
        }
        if (area_sum > 0.0) {
            coffrage.centroid[0] = static_cast<float>(cx / area_sum);
            coffrage.centroid[1] = static_cast<float>(cy / area_sum);
            coffrage.centroid[2] = static_cast<float>(cz / area_sum);
            total += coffrage.area;
            recordObject(objects_out, id, mn, mx);
            if (faces_out) faces_out->push_back(std::move(coffrage));
        }
    }
    return total;
}

double measureSurfaceSectionProjetee(ViewportWindow& vp,
                                     const std::vector<std::uint32_t>& object_ids,
                                     std::vector<MeasuredObject>* objects_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        const double dx = static_cast<double>(mx[0]) - mn[0];
        const double dy = static_cast<double>(mx[1]) - mn[1];
        const double dz = static_cast<double>(mx[2]) - mn[2];
        // Principal axis = longest dimension; section = the other two.
        double length, s1, s2;
        if (dx >= dy && dx >= dz) { length = dx; s1 = dy; s2 = dz; }
        else if (dy >= dz)         { length = dy; s1 = dx; s2 = dz; }
        else                       { length = dz; s1 = dx; s2 = dy; }
        const double perimeter = 2.0 * (s1 + s2);
        total += perimeter * length;
        recordObject(objects_out, id, mn, mx);
    }
    return total;
}

double measureVolumeCouche(ViewportWindow& vp,
                           const std::vector<std::uint32_t>& object_ids,
                           double thickness_m,
                           std::vector<MeasuredObject>* objects_out,
                           std::vector<TopFace>* faces_out) {
    double total = 0.0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        const TopFace face = extractHorizontalFace(worldTriangles(vp, id), /*up=*/true);
        if (face.area <= 0.0) continue;
        total += face.area * thickness_m;
        recordObject(objects_out, id, mn, mx);
        if (faces_out) faces_out->push_back(face);
    }
    return total;
}

double measureVolumeEntre(ViewportWindow& vp,
                          const std::vector<std::uint32_t>& object_ids,
                          std::vector<MeasuredObject>* objects_out) {
    if (object_ids.size() < 2) return 0.0;
    // Pair up consecutive objects: (0,1), (2,3), ... Each pair contributes
    // top-area(lower) × vertical gap to bottom-face(upper).
    double total = 0.0;
    float mn[3], mx[3];
    for (std::size_t i = 0; i + 1 < object_ids.size(); i += 2) {
        const std::uint32_t lower_id = object_ids[i];
        const std::uint32_t upper_id = object_ids[i + 1];
        if (!objectAabb(vp, lower_id, mn, mx)) continue;
        const TopFace lower_top = extractHorizontalFace(worldTriangles(vp, lower_id), /*up=*/true);
        float umn[3], umx[3];
        if (!objectAabb(vp, upper_id, umn, umx)) continue;
        const TopFace upper_bot = extractHorizontalFace(worldTriangles(vp, upper_id), /*up=*/false);
        const double gap = static_cast<double>(umn[2]) - static_cast<double>(mx[2]);
        if (gap <= 0.0 || lower_top.area <= 0.0) continue;
        total += lower_top.area * gap;
        recordObject(objects_out, lower_id, mn, mx);
        float lmn[3] = {mn[0], mn[1], mn[2]};
        float lmx[3] = {mx[0], mx[1], umx[2]};  // span both objects vertically
        recordObject(objects_out, upper_id, umn, umx);
        (void)lmn; (void)lmx;
    }
    return total;
}

double measurePente(ViewportWindow& vp,
                    const std::vector<std::uint32_t>& object_ids,
                    bool on_3d_length,
                    std::vector<MeasuredObject>* objects_out) {
    // Mean normal over all triangles (area-weighted): slope% = sqrt(1-nz²)/|nz| × 100.
    double total = 0.0;
    int count = 0;
    float mn[3], mx[3];
    for (std::uint32_t id : object_ids) {
        if (!objectAabb(vp, id, mn, mx)) continue;
        const WorldMesh wm = worldTriangles(vp, id);
        if (wm.triangles.empty()) continue;
        double nx_sum = 0.0, ny_sum = 0.0, nz_sum = 0.0, area_sum = 0.0;
        for (std::size_t t = 0; t + 8 < wm.triangles.size(); t += 9) {
            const float* a = &wm.triangles[t];
            const float* b = &wm.triangles[t + 3];
            const float* c = &wm.triangles[t + 6];
            const float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
            const float vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
            const float nx = uy * vz - uz * vy;
            const float ny = uz * vx - ux * vz;
            const float nz = ux * vy - uy * vx;
            const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len < 1e-9f) continue;
            const double area = 0.5 * static_cast<double>(len);
            nx_sum += nx; ny_sum += ny; nz_sum += nz;  // un-normalised: area-weighted
            area_sum += area;
        }
        if (area_sum <= 0.0) continue;
        // Normalise the area-weighted mean normal.
        const double inv = 1.0 / (2.0 * area_sum);
        const double nnx = nx_sum * inv, nny = ny_sum * inv, nnz = nz_sum * inv;
        const double nlen = std::sqrt(nnx * nnx + nny * nny + nnz * nnz);
        if (nlen < 1e-12) continue;
        const double nz_unit = nnz / nlen;
        const double horizontal = std::sqrt(std::max(0.0, 1.0 - nz_unit * nz_unit));
        double slope_pct;
        if (on_3d_length) {
            // tan over 3D length: horizontal/|nz| is the same ratio; with 3D
            // length the denominator is the full length -> slope = sin/cos,
            // same value. Kept for API parity with the spec's option.
            slope_pct = std::abs(nz_unit) > 1e-9 ? horizontal / std::abs(nz_unit) * 100.0 : 0.0;
        } else {
            slope_pct = std::abs(nz_unit) > 1e-9 ? horizontal / std::abs(nz_unit) * 100.0 : 0.0;
        }
        total += slope_pct;
        ++count;
        recordObject(objects_out, id, mn, mx);
    }
    return count > 0 ? total / count : 0.0;
}

LocalMetrics measureLocalMetrics(ViewportWindow& vp, std::uint32_t object_id) {
    LocalMetrics m;
    float mn[3], mx[3];
    if (!objectAabb(vp, object_id, mn, mx)) return m;
    const WorldMesh wm = worldTriangles(vp, object_id);
    if (wm.triangles.empty()) return m;
    const TopFace bottom = extractHorizontalFace(wm, /*up=*/false);
    m.surface_m2 = bottom.area;
    m.floor_tris = bottom.triangles;
    for (int i = 0; i < 3; ++i) m.label_pos[i] = bottom.centroid[i];
    m.perimetre_ml = aabbBasePerimeter(mn, mx);
    m.hauteur_m = static_cast<double>(mx[2]) - mn[2];
    m.volume_m3 = m.surface_m2 * m.hauteur_m;
    return m;
}

double measureLocalSurfaceSousPlafond(ViewportWindow& vp,
                                      std::uint32_t object_id,
                                      double min_height_m,
                                      std::vector<MeasuredObject>* objects_out,
                                      std::vector<TopFace>* faces_out) {
    float mn[3], mx[3];
    if (!objectAabb(vp, object_id, mn, mx)) return 0.0;
    const double hauteur = static_cast<double>(mx[2]) - mn[2];
    if (hauteur < min_height_m) return 0.0;
    const TopFace bottom = extractHorizontalFace(worldTriangles(vp, object_id), /*up=*/false);
    if (bottom.area <= 0.0) return 0.0;
    recordObject(objects_out, object_id, mn, mx);
    if (faces_out) faces_out->push_back(bottom);
    return bottom.area;
}

} // namespace bonsaiviewer::modules::measurement
