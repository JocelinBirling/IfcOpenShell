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

TopFace measureTopFace(ViewportWindow& vp, std::uint32_t object_id) {
    TopFace face;
    ViewportWindow::InstanceLookup lookup;
    if (!vp.findInstance(object_id, lookup)) return face;
    ViewportWindow::MeshTriangles mesh;
    if (!vp.readbackMeshTriangles(lookup.session_model_id, lookup.mesh_id, mesh)) return face;
    if (mesh.indices.empty() || mesh.positions.empty()) return face;

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
    std::vector<float> tris;
    float zmin = FLT_MAX, zmax = -FLT_MAX;
    for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
        for (int corner = 0; corner < 3; ++corner) {
            const float* p = &mesh.positions[3 * std::size_t(mesh.indices[t + corner])];
            const Eigen::Vector4d w = world * Eigen::Vector4d(p[0], p[1], p[2], 1.0);
            tris.push_back(static_cast<float>(w.x()));
            tris.push_back(static_cast<float>(w.y()));
            tris.push_back(static_cast<float>(w.z()));
            zmin = std::min(zmin, static_cast<float>(w.z()));
            zmax = std::max(zmax, static_cast<float>(w.z()));
        }
    }
    if (tris.empty()) return face;
    const float tol = std::max(0.01f, (zmax - zmin) * 0.02f);

    double area_sum = 0.0, cx = 0.0, cy = 0.0, cz = 0.0;
    for (std::size_t t = 0; t + 8 < tris.size(); t += 9) {
        const float* a = &tris[t];
        const float* b = &tris[t + 3];
        const float* c = &tris[t + 6];
        const float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
        const float vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
        const float nx = uy * vz - uz * vy;
        const float ny = uz * vx - ux * vz;
        const float nz = ux * vy - uy * vx;
        const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-9f) continue;
        const double area = 0.5 * static_cast<double>(len);
        const float tri_cz = (a[2] + b[2] + c[2]) / 3.0f;
        // Top face: horizontal, pointing up, at the top of the object.
        if (nz / len >= 0.9f && tri_cz >= zmax - tol) {
            face.area += area;
            cx += area * (a[0] + b[0] + c[0]) / 3.0;
            cy += area * (a[1] + b[1] + c[1]) / 3.0;
            cz += area * tri_cz;
            face.triangles.insert(face.triangles.end(), a, a + 9);
            area_sum += area;
        }
    }
    if (area_sum > 0.0) {
        face.centroid[0] = static_cast<float>(cx / area_sum);
        face.centroid[1] = static_cast<float>(cy / area_sum);
        face.centroid[2] = static_cast<float>(cz / area_sum);
    }
    return face;
}

} // namespace bonsaiviewer::modules::measurement
