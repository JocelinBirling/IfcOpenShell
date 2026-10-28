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
 ********************************************************************************/

#ifndef IFCINTERFACE_SERVICES_IDSENGINE_H
#define IFCINTERFACE_SERVICES_IDSENGINE_H

#include "IdsBridge.h"

#include "../ports/ifctester/Ids.h"

#include <optional>
#include <stdexcept>
#include <vector>

// B06 / B24 — the IDS engine of the viewer: the requests of the former
// Python worker (src/ifctester/ifctester_ipc.py), in C++ on the port of
// IfcTester (ports/ifctester). Converts the editor's facets (IdsFacet,
// IdsValue) to IfcTester facets and back, selects, and introspects the
// schema and the model. Synchronous; IdsBridge makes it asynchronous.

namespace bonsaiviewer::services::ids_engine {

namespace it = bonsaiviewer::ports::ifctester;

// A request the user can fix (bad class, bad number...).
struct UserError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// IdsValue <-> IfcTester parameter (None / str / Restriction).
it::Param toParam(const IdsValue& value, const QString& label);
IdsValue fromParam(const it::Param& param);

// One editor facet <-> IfcTester facet.
it::Facet toFacet(const IdsFacet& facet, int index, ifcopenshell::file* file = nullptr);
IdsFacet fromFacet(const it::Facet& facet);

// The selection as one specification: the entity facet in the
// applicability, the others in the requirements.
it::Specification buildSpecification(const QString& name, const std::vector<IdsFacet>& facets,
                                      ifcopenshell::file* file = nullptr);
QString buildXml(const QString& name, const std::vector<IdsFacet>& facets);
// The first specification of an IDS document, as editor facets.
std::vector<IdsFacet> parseXml(const QString& ids_xml, QString* name = nullptr);
std::vector<IdsFacet> facetsOf(const it::Specification& specification);

// Facets as IDS XML for the clipboard (a <facets> element, see
// BonsaiViewer24), and back. The text may also be a whole IDS document or
// a lone facet element.
QString facetsToClipboard(const std::vector<IdsFacet>& facets);
std::vector<IdsFacet> facetsFromClipboard(const QString& text);

// The EXPRESS ids passing every facet of the specification; `limit`
// restricts the candidates.
std::vector<int> select(ifcopenshell::file& file, const it::Specification& specification,
                        const std::optional<std::vector<int>>& limit = std::nullopt);

QStringList selectableClasses(ifcopenshell::file* file);
IdsBridge::ClassInfo classInfo(ifcopenshell::file* file, const IdsValue& name, bool include_subtypes);
void properties(ifcopenshell::file& file, const IdsFacet& entity, IdsBridge::PropertyMap& psets,
                IdsBridge::SampleMap& samples);
// Same, on the elements passing the selection's facets.
void selectionProperties(ifcopenshell::file& file, const std::vector<IdsFacet>& facets,
                         IdsBridge::PropertyMap& psets, IdsBridge::SampleMap& samples);
IdsBridge::ValueStats values(ifcopenshell::file& file, const IdsFacet& entity, const QString& pset,
                             const QString& property);

} // namespace bonsaiviewer::services::ids_engine

#endif // IFCINTERFACE_SERVICES_IDSENGINE_H
