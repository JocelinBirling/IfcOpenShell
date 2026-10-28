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

// PORT — see Classification.h.

#include "Classification.h"

#include "Element.h"

#include <map>

namespace bonsaiviewer::ports::ifcopenshell_util {

// def get_references(element, should_inherit=True)
EntitySet get_references(const Entity& element, bool should_inherit) {
    EntitySet results;
    if (!isA(element, "IfcRoot")) {
        // HasExternalReferences (IFC4) or HasExternalReference (IFC2X3).
        const int plural = attributeCategory(element, "HasExternalReferences");
        const int singular = attributeCategory(element, "HasExternalReference");
        if (plural == 2 || singular == 2) {
            const auto references = inverse(element, plural == 2 ? "HasExternalReferences" : "HasExternalReference");
            for (const Entity& r : references) results.insert(forwardEntity(r, "RelatingReference"));
            return results;
        }
    }
    if (should_inherit && isA(element, "IfcObject")) {
        const Entity element_type = get_type(element);
        if (element_type && element_type != element) results = get_references(element_type);
    }
    EntitySet occurrence_results;
    for (const Entity& r : inverse(element, "HasAssociations")) {
        if (isA(r, "IfcRelAssociatesClassification")) occurrence_results.insert(forwardEntity(r, "RelatingClassification"));
    }
    if (!results.empty()) {
        // The occurrence overrides the type, system by system.
        std::map<Entity, std::vector<Entity>> type_references_per_system;
        std::map<Entity, std::vector<Entity>> occurrence_references_per_system;
        for (const Entity& result : results) type_references_per_system[get_classification(result)].push_back(result);
        for (const Entity& result : occurrence_results) {
            occurrence_references_per_system[get_classification(result)].push_back(result);
        }
        for (const auto& [system, values] : occurrence_references_per_system) type_references_per_system[system] = values;
        EntitySet merged;
        for (const auto& [system, values] : type_references_per_system) merged.insert(values.begin(), values.end());
        return merged;
    }
    return occurrence_results;
}

// def get_inherited_references(reference)
std::vector<Entity> get_inherited_references(Entity reference) {
    std::vector<Entity> results;
    while (true) {
        if (!reference || isA(reference, "IfcClassification")) break;
        results.push_back(reference);
        reference = forwardEntity(reference, "ReferencedSource");
    }
    return results;
}

// def get_classification(reference)
Entity get_classification(const Entity& reference) {
    if (!reference) return {};
    if (isA(reference, "IfcClassification")) return reference;
    const Entity source = forwardEntity(reference, "ReferencedSource");
    return source ? get_classification(source) : Entity();
}

} // namespace bonsaiviewer::ports::ifcopenshell_util
