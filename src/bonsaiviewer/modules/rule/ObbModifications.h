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

#ifndef IFCINTERFACE_MODULES_RULE_OBBMODIFICATIONS_H
#define IFCINTERFACE_MODULES_RULE_OBBMODIFICATIONS_H

#include <QJsonArray>
#include <QString>
#include <QStringList>

#include <vector>

namespace bonsaiviewer::modules::rule::obb {

// BonsaiViewer09 (TI09) — the list_of_modifications of OBB_Custom as rows,
// for its editor (ObbModificationsEditor); Qt Core only, the lines are read
// by the engine's parser (geometry/ObbModifications).
struct Row {
    enum class Kind { Step, NewZone, Raw };
    Kind kind = Kind::Step;
    QString op = QStringLiteral("extend_up"); // Step
    double value = 0.0;                        // Step: metres, or the percentage (50 for 50%)
    bool percent = false;
    QString raw;   // Raw: the entry as written, kept until corrected
    QString error; // Raw: the message of the engine
};

// The entries of the rule file, one row each; an entry the engine refuses
// is kept as a Raw row with its message (nothing is lost).
std::vector<Row> rowsFromJson(const QJsonArray& entries);
// The rows as entries: "extend_up:0.5", "expand_sides:20%", "NEW_OBB", or
// the raw text.
QJsonArray rowsToJson(const std::vector<Row>& rows);
QString entryOf(const Row& row);

// What a step does, in words ("the zone grows 0.5 m upward").
QString describeStep(const QString& op, double value, bool percent);

// The rows the engine would refuse (Raw rows, a NEW_OBB first, last or
// doubled), the first message of the engine, and the zones of the list.
struct Check {
    std::vector<int> bad_rows;
    QString message; // empty: the list is valid
    int zones = 1;
};
Check check(const std::vector<Row>& rows);

} // namespace bonsaiviewer::modules::rule::obb

#endif // IFCINTERFACE_MODULES_RULE_OBBMODIFICATIONS_H
