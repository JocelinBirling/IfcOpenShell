// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
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

#include "ObbModifications.h"

#include "geometry/ObbModifications.h"
#include "core/Errors.h"

#include <cmath>

namespace bonsaiviewer::modules::rule::obb {

namespace geo = ifcclash::geometry;

namespace {

QString number(double value) {
    return QString::number(value, 'g', 12);
}

} // namespace

std::vector<Row> rowsFromJson(const QJsonArray& entries) {
    std::vector<Row> rows;
    for (int i = 0; i < entries.size(); ++i) {
        const QString text = entries.at(i).toString();
        Row row;
        try {
            const geo::ObbStep step = geo::parse_obb_step(text.toStdString(), static_cast<size_t>(i + 1));
            if (step.op == geo::ObbOp::NewObb) {
                row.kind = Row::Kind::NewZone;
            } else {
                row.op = QString::fromStdString(geo::obb_op_name(step.op));
                row.percent = step.change.percent;
                // A percentage is shown as written (50 for 50%), rounded back from the ratio.
                row.value = step.change.percent ? std::round(step.change.value * 100.0 * 1e9) / 1e9 : step.change.value;
            }
        } catch (const ifcclash::ConfigError& e) {
            row.kind = Row::Kind::Raw;
            row.raw = text;
            row.error = QString::fromUtf8(e.what());
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

QString entryOf(const Row& row) {
    switch (row.kind) {
    case Row::Kind::NewZone: return QStringLiteral("NEW_OBB");
    case Row::Kind::Raw: return row.raw;
    case Row::Kind::Step: break;
    }
    return row.op + ':' + number(row.value) + (row.percent ? QStringLiteral("%") : QString());
}

QJsonArray rowsToJson(const std::vector<Row>& rows) {
    QJsonArray entries;
    for (const Row& row : rows) entries.append(entryOf(row));
    return entries;
}

QString describeStep(const QString& op, double value, bool percent) {
    const QString amount = percent ? number(std::abs(value)) + " % of its height" : number(std::abs(value)) + " m";
    const bool shrink = value < 0;
    if (op == QLatin1String("expand_sides")) {
        if (percent) {
            return QString("the two horizontal widths %1 by %2 % (half of %3 %)")
                .arg(shrink ? "shrink" : "grow", number(std::abs(value) / 2), number(std::abs(value)));
        }
        return QString("the two horizontal widths are set to the height of the zone %1 %2 m")
            .arg(shrink ? "-" : "+", number(std::abs(value)));
    }
    if (op == QLatin1String("detach_top_by_extrude") || op == QLatin1String("detach_bottom_by_extrude")) {
        const bool top = op == QLatin1String("detach_top_by_extrude");
        return QString("the zone becomes a slab of %1 %2 it").arg(amount, top ? "on top of" : "below");
    }
    if (op == QLatin1String("extend_up") || op == QLatin1String("extend_down")) {
        const bool up = op == QLatin1String("extend_up");
        if (shrink) return QString("the zone shrinks by %1 from its %2").arg(amount, up ? "top" : "bottom");
        return QString("the zone grows %1 %2").arg(amount, up ? "upward" : "downward");
    }
    return QStringLiteral("unknown operation");
}

Check check(const std::vector<Row>& rows) {
    Check result;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const Row& row = rows[static_cast<size_t>(i)];
        if (row.kind == Row::Kind::Raw) result.bad_rows.push_back(i);
        if (row.kind != Row::Kind::NewZone) continue;
        ++result.zones;
        const bool first = i == 0, last = i + 1 == static_cast<int>(rows.size());
        const bool doubled = !first && rows[static_cast<size_t>(i - 1)].kind == Row::Kind::NewZone;
        if (first || last || doubled) result.bad_rows.push_back(i);
    }
    std::vector<std::string> lines;
    for (const Row& row : rows) lines.push_back(entryOf(row).toStdString());
    try {
        geo::parse_obb_modifications(lines, "OBB_Custom");
    } catch (const ifcclash::ConfigError& e) {
        result.message = QString::fromUtf8(e.what());
    }
    return result;
}

} // namespace bonsaiviewer::modules::rule::obb
