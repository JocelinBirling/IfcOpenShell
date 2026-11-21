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
#include "../../ElementRegistry.h"
#include "../../ports/ifcopenshell_util/Attr.h"
#include "../../ports/ifcopenshell_util/Element.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
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
    // Reuse the lateral grouping from Measurements.cpp via the world mesh.
    const WorldMesh wm = worldTriangles(vp, object_id);
    if (wm.triangles.empty()) return 0.0;

    // Glazing test. Two paths: the IFC material (name / rendering
    // transparency), or — the one that works for projects whose windows
    // carry no IfcMaterial at all, like the Duplex — the transparency baked
    // into the mesh vertex colors (alpha < 255), i.e. exactly what the
    // renderer routes to the transparent pass and what the .ifcview stores.
    bool mesh_has_alpha = false;
    for (const std::uint8_t a : wm.alpha) {
        if (a < 255u) { mesh_has_alpha = true; break; }
    }
    if (!mesh_has_alpha && !elementIsGlazed(element)) return 0.0;

    // A triangle counts as glazed when all three of its vertices are
    // transparent; when the mesh has transparent vertices, only those
    // triangles are grouped (the frame stays out of the measurement).
    std::vector<char> tri_glazed;
    bool any_glazed_tri = false;
    if (mesh_has_alpha && !wm.indices.empty()
        && wm.indices.size() == wm.triangles.size() / 3) {
        tri_glazed.resize(wm.indices.size() / 3);
        for (std::size_t t = 0; t + 2 < wm.indices.size(); t += 3) {
            const auto transparent = [&](std::uint32_t vi) {
                return vi < wm.alpha.size() && wm.alpha[vi] < 255u;
            };
            const char g = (transparent(wm.indices[t]) && transparent(wm.indices[t + 1])
                            && transparent(wm.indices[t + 2])) ? 1 : 0;
            tri_glazed[t / 3] = g;
            if (g) any_glazed_tri = true;
        }
    }
    const bool only_glazed = mesh_has_alpha && any_glazed_tri;

    // Sum the largest coplanar face group of the mesh (the glazed pane is
    // usually the biggest face of a window / curtain wall; it can be the
    // top face of a skylight too).
    Eigen::Vector3f emn, emx;
    if (!vp.computeObjectAabb(object_id, emn, emx)) return 0.0;
    float mn[3] = {emn.x(), emn.y(), emn.z()};
    float mx[3] = {emx.x(), emx.y(), emx.z()};

    // Largest coplanar face group: group by (normal, offset) and keep the largest.
    struct Group {
        float nx = 0, ny = 0, nz = 0, d = 0;
        double area = 0.0, cx = 0.0, cy = 0.0, cz = 0.0;
        std::vector<float> tris;
    };
    std::vector<Group> groups;
    constexpr float kNormalTol = 0.05f;
    constexpr float kPlaneTol = 0.02f;
    for (std::size_t t = 0; t + 8 < wm.triangles.size(); t += 9) {
        if (only_glazed && !tri_glazed[t / 9]) continue;
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

LocalPerimeterResult measureLocalPerimetre(ViewportWindow& vp,
                                            bonsaiviewer::ElementRegistry& registry,
                                            std::uint32_t space_object_id) {
    LocalPerimeterResult result;
    Eigen::Vector3f smn, smx;
    if (!vp.computeObjectAabb(space_object_id, smn, smx)) return result;
    const float space_zmin = smn.z();
    // Raw base perimeter of the space (AABB footprint, Z up).
    const double dx = static_cast<double>(smx.x()) - smn.x();
    const double dy = static_cast<double>(smx.y()) - smn.y();
    result.perimetre_ml = 2.0 * (dx + dy);

    // Scan the model's elements for IfcDoor / IfcWindow whose footprint
    // intersects the space's base rectangle and whose bottom is at floor
    // level (|z_min - space_zmin| <= 10 cm). Deduct the narrower horizontal
    // dimension of the opening's AABB (the door/window width).
    const auto info = registry.findBasicElementInfo(space_object_id);
    if (!info) return result;
    constexpr float kFloorTol = 0.10f;
    for (const auto& el : registry.basicElementInfoForModel(info->session_model_id)) {
        if (el.type != QStringLiteral("IfcDoor") && el.type != QStringLiteral("IfcWindow")) continue;
        Eigen::Vector3f emn, emx;
        if (!vp.computeObjectAabb(el.object_id, emn, emx)) continue;
        // Floor-touching: the opening's bottom is near the space's floor.
        if (std::abs(emn.z() - space_zmin) > kFloorTol) continue;
        // Footprint intersects the space's base rectangle (XY overlap).
        const bool overlap_x = emn.x() < smx.x() && emx.x() > smn.x();
        const bool overlap_y = emn.y() < smx.y() && emx.y() > smn.y();
        if (!overlap_x || !overlap_y) continue;
        const double w = static_cast<double>(emx.x()) - emn.x();
        const double h = static_cast<double>(emx.y()) - emn.y();
        result.deductions_ml += std::min(w, h);
    }
    result.net_ml = std::max(0.0, result.perimetre_ml - result.deductions_ml);
    return result;
}

SousPlafondResult measureLocalSousPlafond(ViewportWindow& vp,
                                          bonsaiviewer::ElementRegistry& registry,
                                          std::uint32_t space_object_id,
                                          double surface_m2) {
    SousPlafondResult result;
    Eigen::Vector3f smn, smx;
    if (!vp.computeObjectAabb(space_object_id, smn, smx)) return result;
    const float space_zmin = smn.z();
    const auto info = registry.findBasicElementInfo(space_object_id);
    if (!info) return result;

    // Find the lowest ceiling-like element above the space's floor whose
    // footprint overlaps the space's base rectangle.
    float best_zmin = FLT_MAX;
    std::uint32_t best_id = 0;
    for (const auto& el : registry.basicElementInfoForModel(info->session_model_id)) {
        const bool is_covering = el.type == QStringLiteral("IfcCovering");
        const std::string name_l = lower(el.name.toStdString());
        const bool named_ceiling = name_l.find("plafond") != std::string::npos ||
                                   name_l.find("ceiling") != std::string::npos;
        if (!is_covering && !named_ceiling) continue;
        Eigen::Vector3f emn, emx;
        if (!vp.computeObjectAabb(el.object_id, emn, emx)) continue;
        // The element must be above the space's floor.
        if (emn.z() <= space_zmin + 0.05f) continue;
        // Footprint overlap in XY.
        const bool overlap_x = emn.x() < smx.x() && emx.x() > smn.x();
        const bool overlap_y = emn.y() < smx.y() && emx.y() > smn.y();
        if (!overlap_x || !overlap_y) continue;
        if (emn.z() < best_zmin) {
            best_zmin = emn.z();
            best_id = el.object_id;
        }
    }
    if (best_id != 0) {
        result.found = true;
        result.plafond_object_id = best_id;
        result.hauteur_m = static_cast<double>(best_zmin) - space_zmin;
        result.volume_m3 = surface_m2 * result.hauteur_m;
    }
    return result;
}

std::vector<std::pair<QString, QString>> measureIdList() {
    return {
        {QStringLiteral("M0"),  QStringLiteral("M0 — Aire face sup")},
        {QStringLiteral("M1"),  QStringLiteral("M1 — Compter")},
        {QStringLiteral("M2"),  QStringLiteral("M2 — Longueur d'axe")},
        {QStringLiteral("M3"),  QStringLiteral("M3 — Périmètre")},
        {QStringLiteral("M4"),  QStringLiteral("M4 — Longueur d'arête")},
        {QStringLiteral("M5"),  QStringLiteral("M5 — Main courante")},
        {QStringLiteral("M6"),  QStringLiteral("M6 — Surface top")},
        {QStringLiteral("M7"),  QStringLiteral("M7 — Surface bottom")},
        {QStringLiteral("M8"),  QStringLiteral("M8 — Surface latérale")},
        {QStringLiteral("M9"),  QStringLiteral("M9 — Surface vitrage")},
        {QStringLiteral("M11"), QStringLiteral("M11 — Toiture développée")},
        {QStringLiteral("M12"), QStringLiteral("M12 — Surface coffrage")},
        {QStringLiteral("M13"), QStringLiteral("M13 — Section projetée")},
        {QStringLiteral("M16"), QStringLiteral("M16 — Surface ouverture")},
        {QStringLiteral("M17"), QStringLiteral("M17 — Volume élément")},
        {QStringLiteral("M18"), QStringLiteral("M18 — Volume de couche")},
        {QStringLiteral("M21"), QStringLiteral("M21 — Local surface sol")},
        {QStringLiteral("M22"), QStringLiteral("M22 — Surface sous plafond")},
        {QStringLiteral("M27"), QStringLiteral("M27 — Pente")},
        {QStringLiteral("M27b"), QStringLiteral("M27b — Pente (ports)")},
    };
}

MeasureDispatchResult runMeasureById(ViewportWindow& vp,
                                     bonsaiviewer::ElementRegistry& registry,
                                     std::uint32_t object_id,
                                     const QString& measure_id) {
    MeasureDispatchResult r;
    const std::vector<std::uint32_t> one{object_id};
    const auto entity = registry.findEntity(object_id);
    if (measure_id == QLatin1String("M0")) {
        const TopFace f = measureTopFace(vp, object_id);
        r.value = f.area; r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M1")) {
        r.value = 1.0; r.unit = QStringLiteral("U");
    } else if (measure_id == QLatin1String("M2")) {
        r.value = measureLongueurAxe(vp, one); r.unit = QStringLiteral("ml");
    } else if (measure_id == QLatin1String("M3")) {
        r.value = measurePerimetre(vp, one); r.unit = QStringLiteral("ml");
    } else if (measure_id == QLatin1String("M4")) {
        r.value = measureLongueurArete(vp, one); r.unit = QStringLiteral("ml");
    } else if (measure_id == QLatin1String("M5")) {
        r.value = measureLongueurMainCourante(vp, one); r.unit = QStringLiteral("ml");
    } else if (measure_id == QLatin1String("M6")) {
        r.value = measureSurfaceTop(vp, one); r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M7")) {
        r.value = measureSurfaceBottom(vp, one); r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M8")) {
        r.value = measureSurfaceLateral(vp, one, 0); r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M9")) {
        if (entity) r.value = measureSurfaceVitrage(vp, *entity, object_id);
        r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M11")) {
        r.value = measureSurfaceToitureDeveloppee(vp, one); r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M12")) {
        r.value = measureSurfaceCoffrage(vp, one, false, false); r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M13")) {
        r.value = measureSurfaceSectionProjetee(vp, one); r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M16")) {
        if (entity) r.value = measureSurfaceOuverture(vp, *entity, object_id).area_m2;
        r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M17")) {
        r.value = measureVolumeElement(vp, one); r.unit = QStringLiteral("m³");
    } else if (measure_id == QLatin1String("M18")) {
        r.value = measureVolumeCouche(vp, one, 0.10); r.unit = QStringLiteral("m³");
    } else if (measure_id == QLatin1String("M21")) {
        const LocalMetrics m = measureLocalMetrics(vp, object_id);
        r.value = m.surface_m2; r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M22")) {
        r.value = measureLocalSurfaceSousPlafond(vp, object_id, 1.80); r.unit = QStringLiteral("m²");
    } else if (measure_id == QLatin1String("M27")) {
        r.value = measurePente(vp, one, false); r.unit = QStringLiteral("%");
    } else if (measure_id == QLatin1String("M27b")) {
        if (entity) {
            const auto s = measurePentePorts(*entity);
            if (s.has_value()) r.value = *s;
        }
        r.unit = QStringLiteral("%");
    }
    return r;
}

} // namespace bonsaiviewer::modules::measurement
