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

#ifndef IFCINTERFACE_SERVICES_RELATIONVALUES_H
#define IFCINTERFACE_SERVICES_RELATIONVALUES_H

#include "../ifcparse/express.h"

#include <QString>

#include <vector>

namespace bonsaiviewer::services::relations {

// BonsaiViewer06 — what an element is related to, as a text: the value an
// Automatic Color by relation groups the elements on. Keys (stored in the
// filter files) and labels:
//   model           Model           the IFC file name
//   storey          Level           IfcBuildingStorey (contained or referenced)
//   building        Building        IfcBuilding
//   space           Space           IfcSpace
//   system          System          IfcSystem (not zones), by group assignment
//   zone            Zone            IfcZone, of the element or of its space
//   group           Group           IfcGroup other than systems and zones
//   material        Material        the materials (a set lists its materials)
//   type            Type            the element type
//   classification  Classification  the classification references
//   class           Class           the IFC class
struct Relation {
    QString key;
    QString label;
};
const std::vector<Relation>& all();
// The label of a key ("Level" for "storey"); the key itself when unknown.
QString label(const QString& key);

// The names of the related elements, sorted, without duplicates, joined
// with ", ". Empty when the element has no such relation. `model_name` is
// the value of the "model" relation.
QString value(const express::base& element, const QString& key, const QString& model_name);

} // namespace bonsaiviewer::services::relations

#endif // IFCINTERFACE_SERVICES_RELATIONVALUES_H
