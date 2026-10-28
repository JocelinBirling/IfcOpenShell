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

// PORT of src/ifctester/ifctester/ids.py — Ids, Specification: the IDS 1.0
// XML read / write (Qt XML stream classes instead of xmlschema; the XML is
// not validated against ids.xsd) and Specification.validate().

#ifndef BONSAIVIEWER_PORTS_IFCTESTER_IDS_H
#define BONSAIVIEWER_PORTS_IFCTESTER_IDS_H

#include "Facets.h"

#include <QByteArray>

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace bonsaiviewer::ports::ifctester {

struct FacetFailure {
    Entity element;
    std::string reason;
};

// The validation state Python keeps on each requirement facet
// (facet.status, facet.passed_entities, facet.failures).
struct FacetOutcome {
    std::optional<bool> status;
    std::set<Entity> passed_entities;
    std::vector<FacetFailure> failures;
};

struct Specification {
    std::string name = "Unnamed";
    std::vector<Facet> applicability;
    std::vector<Facet> requirements;
    int minOccurs = 0;
    std::optional<int> maxOccurs;  // nullopt: "unbounded"
    std::vector<std::string> ifcVersion = {"IFC2X3", "IFC4", "IFC4X3_ADD2"};
    std::optional<std::string> identifier;
    std::optional<std::string> description;
    std::optional<std::string> instructions;

    // Filled by validate().
    std::vector<Entity> applicable_entities;
    std::set<Entity> passed_entities;
    std::set<Entity> failed_entities;
    std::vector<FacetOutcome> requirement_outcomes;  // one per requirement
    std::optional<bool> status;
    std::optional<bool> is_ifc_version;

    bool prohibited() const { return maxOccurs && *maxOccurs == 0; }

    void reset_status();
    bool check_ifc_version(ifcopenshell::file& file);
    void validate(ifcopenshell::file& file, bool should_filter_version = false);

    Cardinality get_usage() const;
    void set_usage(Cardinality usage);
};

struct Ids {
    // info: title, copyright, version, description, author, date, purpose,
    // milestone — in that order, only the ones set.
    std::vector<std::pair<std::string, std::string>> info = {{"title", "Untitled"}};
    std::vector<Specification> specifications;

    std::optional<std::string> infoValue(const std::string& key) const;
    void setInfo(const std::string& key, const std::string& value);  // empty removes

    void validate(ifcopenshell::file& file, bool should_filter_version = false);
};

// ids.from_string(xml). nullopt (and `error`) when the XML is not an IDS.
std::optional<Ids> idsFromXml(const QByteArray& xml, std::string* error = nullptr);
// Ids.to_string()
QByteArray idsToXml(const Ids& ids);

// Facets as an IDS XML fragment (<facets> holding IDS facet elements), and
// back: the clipboard format of the facet copy / paste between Tables,
// Filters and the IDS panel. facetsFromXml also reads a whole IDS document
// (all the facets of its first specification) or a lone facet element.
QByteArray facetsToXml(const std::vector<Facet>& facets, Clause clause = Clause::Requirement);
std::optional<std::vector<Facet>> facetsFromXml(const QByteArray& xml);

} // namespace bonsaiviewer::ports::ifctester

#endif
