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

// The text of the measures of a result (declared in Results.h), apart from
// the widgets so the tests can use it.
#include "Results.h"

namespace bonsaiviewer::modules::rule {

QString measureUnit(ifcclash::Measure measure) {
    switch (measure) {
    case ifcclash::Measure::Distance:
    case ifcclash::Measure::PenetrationDepth:
    case ifcclash::Measure::Offset: return QStringLiteral("m");
    case ifcclash::Measure::ContactArea:
    case ifcclash::Measure::Area: return QStringLiteral("m²");
    case ifcclash::Measure::Volume: return QStringLiteral("m³");
    case ifcclash::Measure::Angle: return QStringLiteral("°");
    case ifcclash::Measure::Ratio:
    case ifcclash::Measure::HitRatio: return QString();
    }
    return {};
}

QString measureText(const ifcclash::ResultRecord& record) {
    if (record.measures.empty()) return {};
    const auto& [measure, value] = record.measures.items().front();
    const QString unit = measureUnit(measure);
    return QString("%1 = %2%3").arg(QString::fromLatin1(ifcclash::measure_name(measure)))
        .arg(value, 0, 'g', 4)
        .arg(unit.isEmpty() ? QString() : " " + unit);
}

} // namespace bonsaiviewer::modules::rule
