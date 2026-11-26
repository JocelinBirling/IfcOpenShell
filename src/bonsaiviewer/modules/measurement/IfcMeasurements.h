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

#ifndef IFCINTERFACE_MODULES_MEASUREMENT_IFCMEASUREMENTS_H
#define IFCINTERFACE_MODULES_MEASUREMENT_IFCMEASUREMENTS_H

#include "Measurements.h"

#include <QString>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

// Forward declarations to keep the IFC headers out of this header.
namespace express { class base; }
class ViewportWindow;
namespace bonsaiviewer { class ElementRegistry; }

namespace bonsaiviewer::modules::measurement {

// BonsaiViewer50 — Mesure Plus, IFC-dependent functions (M9, M16, M27-ports).
// These read the IFC relations (materials, openings, port connections) via
// the entity passed in; the caller resolves object_id -> express::base
// through ElementRegistry::findEntity. Geometry is still recomputed from
// the 3D mesh — IfcQuantity* values are never used.

// M9 — surface_vitrage: area (m²) of the transparent faces of an object.
// Detects transparency from the object's material (name contains
// "verre"/"glass"/"vitrage", or a presentation style with Transparency > 0),
// then sums the area of the largest coplanar lateral face group of the mesh.
// Returns 0 when the object has no transparent material.
double measureSurfaceVitrage(ViewportWindow& vp,
                             const express::base& element,
                             std::uint32_t object_id,
                             std::vector<MeasuredObject>* objects_out = nullptr,
                             std::vector<ClassifiedFace>* faces_out = nullptr);

// M16 — one measure per opening: the surface (m²) of each IfcOpeningElement
// voided in the host, with the opening's position expressed in the host
// object's OBB frame (metres along each OBB axis from its center — for a
// wall: along length / height / thickness).
struct OpeningMeasure {
    QString name;                       // opening Name (may be empty)
    double area_m2 = 0.0;               // 2D representation rectangle area
    float obb_pos[3] = {0, 0, 0};       // position in the host OBB frame
    bool has_obb_pos = false;
};
std::vector<OpeningMeasure> measureOuverturesObb(ViewportWindow& vp,
                                                  const express::base& host,
                                                  std::uint32_t host_object_id,
                                                  std::vector<MeasuredObject>* objects_out = nullptr);

// M27 — pente via connected ports: slope (%) between the two ports of the
// equipment connected through IfcRelConnectsPorts (Δz / horizontal distance
// × 100). Returns nullopt when no port connection is found.
std::optional<double> measurePentePorts(const express::base& element);

// M21 — local_périmètre: base perimeter (ml) of an IfcSpace minus the
// widths of the doors and floor-touching windows whose footprint intersects
// the space's base. Doors / windows are found by scanning the model's
// elements (IfcDoor / IfcWindow) through the ElementRegistry and testing
// their world AABB against the space's base rectangle at floor level.
struct LocalPerimeterResult {
    double perimetre_ml = 0.0;   // raw base perimeter of the space
    double deductions_ml = 0.0;  // summed widths of doors / floor windows
    double net_ml = 0.0;         // perimetre - deductions
};
LocalPerimeterResult measureLocalPerimetre(ViewportWindow& vp,
                                            bonsaiviewer::ElementRegistry& registry,
                                            std::uint32_t space_object_id);

// M21 — local_hauteur_sous_plafond / local_volume_sous_plafond: find the
// lowest ceiling-like element (IfcCovering, or name containing
// "plafond"/"ceiling") whose footprint overlaps the space's base and whose
// bottom is above the space's floor. hauteur = z_min(plafond) - z_min(space);
// volume = surface_m2 (passed in, from measureLocalMetrics) × hauteur.
struct SousPlafondResult {
    double hauteur_m = 0.0;
    double volume_m3 = 0.0;
    std::uint32_t plafond_object_id = 0;
    bool found = false;
};
SousPlafondResult measureLocalSousPlafond(ViewportWindow& vp,
                                          bonsaiviewer::ElementRegistry& registry,
                                          std::uint32_t space_object_id,
                                          double surface_m2);

// Dispatch one measurement function by its id ("M0".."M27b") on a single
// object. Used by the table columns (one row = one element). `unit_out`
// receives the unit ("m²", "ml", "m³", "U", "%", "m"). Returns 0 when the
// measure id is unknown or the object cannot be measured.
struct MeasureDispatchResult {
    double value = 0.0;
    QString unit;
};
MeasureDispatchResult runMeasureById(ViewportWindow& vp,
                                     bonsaiviewer::ElementRegistry& registry,
                                     std::uint32_t object_id,
                                     const QString& measure_id);

// The list of measure ids that can be used as table columns
// (single-object functions; M20 needs two objects and is excluded).
std::vector<std::pair<QString, QString>> measureIdList();  // (id, label)

} // namespace bonsaiviewer::modules::measurement

#endif // IFCINTERFACE_MODULES_MEASUREMENT_IFCMEASUREMENTS_H
