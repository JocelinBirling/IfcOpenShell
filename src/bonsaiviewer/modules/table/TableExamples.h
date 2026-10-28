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

#ifndef IFCINTERFACE_MODULES_TABLE_TABLEEXAMPLES_H
#define IFCINTERFACE_MODULES_TABLE_TABLEEXAMPLES_H

#include "Table.h"

#include <vector>

namespace bonsaiviewer::modules::table {

// BonsaiViewer07 — the example tables stored at the first start (no
// tables.json yet), in the "Examples" folder. Each one shows a feature of
// the tables; its description explains it.
std::vector<TableDefinition> exampleTables();

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_TABLEEXAMPLES_H
