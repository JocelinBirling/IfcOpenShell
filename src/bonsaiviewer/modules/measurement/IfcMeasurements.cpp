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

#include "IfcMeasurements.h"

#include "../../../ifcviewer/ViewportWindow.h"
#include "../../../ifcparse/express.h"
#include "../../ports/ifcopenshell_util/Attr.h"
#include "../../ports/ifcopenshell_util/Element.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace bonsaiviewer::modules::measurement {

namespace util = bonsaiviewer::ports::ifcopenshell_util;

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// True when the entity's material looks transparent: the material name or
// any of its presentation style names contains a glass keyword, or a
// surface-style rendering carries a Transparency > 0.
bool materialIsTransparent(const util::Entity& material) {
    if (!material) return false;
    auto name = util::forwardString(material, "Name");
    if (name) {
        const std::string n = lower(*name);
        if (n.find("verre") != std::string::npos || n.find("glass") != std::string::npos ||
            n.find("vitrage") != std::string::npos || n.find("transparent") != std::string::npos) {
            return true;
        }
    }
    // Walk HasPresentation -> Representations -> Items -> SurfaceStyle ->
    // RenderingStyle -> Transparency.
    for (const auto& rel : util::inverse(material, "HasPresentation")) {
        for (const auto& rep : util::forwardEntities(rel, "Representations")) {
            for (const auto& item : util::forwardEntities(rep, "Items")) {
                auto style = util::forwardEntity(item, "Style");
                if (!style) continue;
                auto rendering = util::forwardEntity(style, "RenderingStyle");
                if (!rendering) continue;
                auto tr = util::forward(rendering, "Transparency");
                if (tr && !tr->isNull()) {
                    try {
                        const double t = static_cast<double>(*tr);
                        if (t > 0.0) return true;
                    } catch (...) {}
                }
            }
        }
    }
    return false;
}

// Object is glazed when its own material (or an inherited one) is transparent.
bool elementIsGlazed(const util::Entity& element) {
    const util::Entity mat = util::get_material(element);
    if (mat && materialIsTransparent(mat)) return true;
    // Also check the type object's material.
    const util::Entity type = util::get_type(element);
    if (type) {
        const util::Entity tmat = util::get_material(type);
        if (tmat && materialIsTransparent(tmat)) return true;
    }
    return false;
}

} // namespace

double measureSurfaceVitrage(ViewportWindow& vp,
                             const express::base& element,
                             std::uint32_t object_id,
                             std::vector<MeasuredObject>* objects_out,
                             std::vector<ClassifiedFace>* faces_out) {
    if (!elementIsGlazed(element)) return 0.0;

    // Sum the largest coplanar lateral face group of the mesh (the glazed
    // pane is usually the biggest lateral face of a window / curtain wall).
    Eigen::Vector3f emn, emx;
    if (!vp.computeObjectAabb(object_id, emn, emx)) return 0.0;
    float mn[3] = {emn.x(), emn.y(), emn.z()};
    float mx[3] = {emx.x(), emx.y(), emx.z()};

    // Reuse the lateral grouping from Measurements.cpp via the world mesh.
    const WorldMesh wm = worldTriangles(vp, object_id);
    if (wm.triangles.empty()) return 0.0;

    // Largest coplanar face group among ALL faces (glazing can be the top
    // face of a skylight too): group by (normal, offset) and keep the largest.
    struct Group {
        float nx = 0, ny = 0, nz = 0, d = 0;
        double area = 0.0, cx = 0.0, cy = 0.0, cz = 0.0;
        std::vector<float> tris;
    };
    std::vector<Group> groups;
    constexpr float kNormalTol = 0.05f;
    constexpr float kPlaneTol = 0.02f;
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
        const float nnx = nx / len, nny = ny / len, nnz = nz / len;
        const float d = nnx * a[0] + nny * a[1] + nnz * a[2];
        Group* g = nullptr;
        for (auto& cand : groups) {
            const float dot = cand.nx * nnx + cand.ny * nny + cand.nz * nnz;
            if (dot >= 1.0f - kNormalTol && std::abs(cand.d - d) <= kPlaneTol) { g = &cand; break; }
        }
        if (!g) {
            groups.push_back(Group{});
            g = &groups.back();
            g->nx = nnx; g->ny = nny; g->nz = nnz; g->d = d;
        }
        g->area += area;
        g->cx += area * (a[0] + b[0] + c[0]) / 3.0;
        g->cy += area * (a[1] + b[1] + c[1]) / 3.0;
        g->cz += area * (a[2] + b[2] + c[2]) / 3.0;
        g->tris.insert(g->tris.end(), a, a + 9);
    }
    if (groups.empty()) return 0.0;
    auto best = std::max_element(groups.begin(), groups.end(),
                                 [](const Group& x, const Group& y) { return x.area < y.area; });

    MeasuredObject obj;
    obj.id = object_id;
    for (int i = 0; i < 3; ++i) { obj.mn[i] = mn[i]; obj.mx[i] = mx[i]; }
    if (objects_out) objects_out->push_back(obj);
    if (faces_out) {
        ClassifiedFace f;
        f.area = best->area;
        f.centroid[0] = static_cast<float>(best->cx / best->area);
        f.centroid[1] = static_cast<float>(best->cy / best->area);
        f.centroid[2] = static_cast<float>(best->cz / best->area);
        f.triangles = best->tris;
        faces_out->push_back(std::move(f));
    }
    return best->area;
}

OpeningResult measureSurfaceOuverture(ViewportWindow& vp,
                                      const express::base& element,
                                      std::uint32_t object_id,
                                      std::vector<MeasuredObject>* objects_out) {
    OpeningResult result;
    // IfcRelVoidsElement: inverse "HasOpenings" on the host element.
    for (const auto& rel : util::inverse(element, "HasOpenings")) {
        const auto opening = util::forwardEntity(rel, "RelatedOpeningElement");
        if (!opening) continue;
        // Try to read the opening's rectangle from its representation:
        // Representation -> Representations -> Items -> Boundaries -> PolyLoop
        // points (2D). Area = |cross| of the rectangle; fall back to the
        // opening's own object AABB when it exists in the viewport.
        double w = 0.0;  // reused as the shoelace area
        auto rep = util::forwardEntity(opening, "Representation");
        if (rep) {
            for (const auto& r : util::forwardEntities(rep, "Representations")) {
                for (const auto& item : util::forwardEntities(r, "Items")) {
                    for (const auto& bnd : util::forwardEntities(item, "Boundaries")) {
                        auto loop = util::forwardEntity(bnd, "Boundaries");
                        // IfcPolyLoop: Points is an aggregate of IfcCartesianPoint (2D).
                        auto pts = util::forwardEntities(loop ? loop : bnd, "Points");
                        std::vector<std::pair<double, double>> xy;
                        for (const auto& p : pts) {
                            auto coords = util::forward(p, "Coordinates");
                            if (!coords || coords->isNull()) continue;
                            try {
                                const auto vec = static_cast<std::vector<double>>(*coords);
                                if (vec.size() >= 2) xy.emplace_back(vec[0], vec[1]);
                            } catch (...) {}
                        }
                        if (xy.size() >= 3) {
                            // Shoelace area of the 2D polygon.
                            double a = 0.0;
                            for (std::size_t i = 0; i < xy.size(); ++i) {
                                const auto& p = xy[i];
                                const auto& q = xy[(i + 1) % xy.size()];
                                a += p.first * q.second - q.first * p.second;
                            }
                            w = std::abs(a) / 2.0;  // shoelace area of the 2D loop
                        }
                    }
                }
            }
        }
        if (w > 0.0) {
            result.area_m2 += w;
            ++result.count;
        } else {
            // Fallback: use the host element's own AABB opening footprint —
            // without the opening's mesh we approximate by counting one
            // opening with the element's thickness × height is not possible,
            // so we just count it and leave area to the mesh path later.
            ++result.count;
        }
    }
    if (objects_out && result.count > 0) {
        Eigen::Vector3f emn, emx;
        if (vp.computeObjectAabb(object_id, emn, emx)) {
            MeasuredObject obj;
            obj.id = object_id;
            obj.mn[0] = emn.x(); obj.mn[1] = emn.y(); obj.mn[2] = emn.z();
            obj.mx[0] = emx.x(); obj.mx[1] = emx.y(); obj.mx[2] = emx.z();
            objects_out->push_back(obj);
        }
    }
    return result;
}

std::optional<double> measurePentePorts(const express::base& element) {
    // IfcRelConnectsPorts: inverse "ConnectedBy" or forward "Ports" on the
    // element; take the two ports' positions (ObjectPlacement relative to
    // the element) and compute Δz / horizontal distance × 100.
    std::vector<std::pair<double, double>> ports;  // (x, z) in element-local
    auto collect_port = [&](const util::Entity& port) {
        if (!port) return;
        auto placement = util::forwardEntity(port, "ObjectPlacement");
        if (!placement) return;
        auto rel = util::forwardEntity(placement, "RelativePlacement");
        if (!rel) return;
        auto loc = util::forward(rel, "Location");
        if (!loc || loc->isNull()) return;
        try {
            const auto c = static_cast<std::vector<double>>(*loc);
            if (c.size() >= 3) ports.emplace_back(c[0], c[2]);
            else if (c.size() >= 1) ports.emplace_back(c[0], 0.0);
        } catch (...) {}
    };
    for (const auto& rel : util::inverse(element, "ConnectedBy")) {
        for (const auto& p : util::forwardEntities(rel, "RelatingPort")) collect_port(p);
        for (const auto& p : util::forwardEntities(rel, "RelatedPort")) collect_port(p);
    }
    // Also walk the element's own ports (IfcDistributionPort via HasPorts).
    for (const auto& rel : util::inverse(element, "HasPorts")) {
        const auto port = util::forwardEntity(rel, "RelatingPort");
        collect_port(port);
    }
    if (ports.size() < 2) return std::nullopt;
    const double dz = std::abs(ports[1].second - ports[0].second);
    const double dx = std::abs(ports[1].first - ports[0].first);
    if (dx < 1e-9) return std::nullopt;
    return dz / dx * 100.0;
}

} // namespace bonsaiviewer::modules::measurement
