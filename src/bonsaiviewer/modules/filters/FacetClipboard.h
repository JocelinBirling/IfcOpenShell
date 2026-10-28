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

#ifndef IFCINTERFACE_MODULES_FILTERS_FACETCLIPBOARD_H
#define IFCINTERFACE_MODULES_FILTERS_FACETCLIPBOARD_H

#include "../../services/IdsBridge.h"

#include <vector>

class QWidget;

// B24 — copy / paste of IDS facets between the tables, the filters and the
// IDS panel. The clipboard holds IDS XML (a <facets> element with the facet
// elements), so a facet can also be pasted into a text editor or an .ids
// file, and an IDS document can be pasted as facets.

namespace bonsaiviewer::modules::filters::facet_clipboard {

// Copies the facets; on an incomplete facet, says why and copies nothing.
bool copy(QWidget* parent, const std::vector<bonsaiviewer::services::IdsFacet>& facets);
// The facets of the clipboard; empty (after a message) when there are none.
std::vector<bonsaiviewer::services::IdsFacet> paste(QWidget* parent);
// The clipboard looks like IDS facets (cheap test, for enabling Paste).
bool canPaste();

} // namespace bonsaiviewer::modules::filters::facet_clipboard

#endif // IFCINTERFACE_MODULES_FILTERS_FACETCLIPBOARD_H
