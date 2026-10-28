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

// PORT of src/ifcopenshell-python/ifcopenshell/util/classification.py —
// get_references, get_inherited_references, get_classification.

#ifndef BONSAIVIEWER_PORTS_IFCOPENSHELL_UTIL_CLASSIFICATION_H
#define BONSAIVIEWER_PORTS_IFCOPENSHELL_UTIL_CLASSIFICATION_H

#include "Attr.h"

#include <set>
#include <vector>

namespace bonsaiviewer::ports::ifcopenshell_util {

// A Python set of entity instances (ordered by instance, like a set: the
// order is not meaningful).
using EntitySet = std::set<Entity>;

EntitySet get_references(const Entity& element, bool should_inherit = true);
std::vector<Entity> get_inherited_references(Entity reference);
Entity get_classification(const Entity& reference);

} // namespace bonsaiviewer::ports::ifcopenshell_util

#endif
