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

#ifndef IFCINTERFACE_SERVICES_FILTEREXAMPLES_H
#define IFCINTERFACE_SERVICES_FILTEREXAMPLES_H

#include "FilterStore.h"

#include <vector>

namespace bonsaiviewer::services {

// BonsaiViewer06 — the example filters stored at the first start (no
// filters.json yet), in the "Examples" folder. Each one shows a feature of
// the filters; its description explains it. `folder_id` is set on each.
std::vector<FilterDocument> exampleFilters(const QString& folder_id);

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_FILTEREXAMPLES_H
