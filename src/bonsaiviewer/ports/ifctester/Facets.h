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

// PORT of src/ifctester/ifctester/facet.py — the six IDS facets (Entity,
// Attribute, Classification, PartOf, Property, Material): filter(),
// __call__() and to_string(), with the Result.to_string() messages.
//
// One struct holds the parameters of every facet type; each facet type only
// reads its own (see the Python `parameters` lists, quoted per field).

#ifndef BONSAIVIEWER_PORTS_IFCTESTER_FACETS_H
#define BONSAIVIEWER_PORTS_IFCTESTER_FACETS_H

#include "Restriction.h"

#include "../../../ifcparse/express.h"

#include <optional>
#include <string>
#include <vector>

namespace ifcopenshell { class file; }

namespace bonsaiviewer::ports::ifctester {

using Entity = express::base;

enum class FacetType { Entity, Attribute, Classification, PartOf, Property, Material };
enum class Cardinality { Required, Optional, Prohibited };
enum class Clause { Applicability, Requirement };

// "entity", "attribute", "classification", "partOf", "property", "material".
std::string facetTypeName(FacetType type);
std::optional<FacetType> facetTypeFromName(const std::string& name);
std::string cardinalityName(Cardinality cardinality);
std::optional<Cardinality> cardinalityFromName(const std::string& name);

// facet.py: class Result — bool(result) and str(result).
struct Result {
    bool is_pass = true;
    std::string reason;  // str(result): empty when passing
    explicit operator bool() const { return is_pass; }
};

struct Facet {
    FacetType type = FacetType::Entity;

    // Entity:         name, predefinedType, @instructions
    // Attribute:      name, value, @cardinality, @instructions
    // Classification: value, system, @uri, @cardinality, @instructions
    // PartOf:         name, predefinedType, @relation, @cardinality, @instructions
    // Property:       propertySet, baseName, value, @dataType, @uri, @cardinality, @instructions
    // Material:       value, @uri, @cardinality, @instructions
    Param name;
    Param predefinedType;
    Param value;
    Param system;
    Param propertySet;
    Param baseName;
    std::optional<std::string> relation;  // "IFCRELAGGREGATES"...
    std::optional<std::string> dataType;
    std::optional<std::string> uri;
    std::optional<std::string> instructions;
    Cardinality cardinality = Cardinality::Required;

    // The Python constructor defaults of each facet type.
    static Facet make(FacetType type);

    // Facet.filter(ifc_file, elements): the broadphase. `elements` null is
    // Python None (first applicability facet).
    std::vector<Entity> filter(ifcopenshell::file& file, const std::vector<Entity>* elements) const;

    // Facet.__call__(inst)
    Result operator()(const Entity& inst) const;

    // Facet.to_string(clause_type, specification, requirement):
    // `specification_prohibited` is specification.maxOccurs == 0.
    std::string toString(Clause clause, bool specification_prohibited = false,
                         const Facet* requirement = nullptr) const;
};

} // namespace bonsaiviewer::ports::ifctester

#endif
