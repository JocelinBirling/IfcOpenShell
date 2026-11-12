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

// BonsaiViewer09 (TI09) — the rule file JSON, the format of
// ifcclash_plus/serialization.py: a file saved by the viewer loads in the
// Python CLI (load_from_json) and the other way round. The viewer adds a
// "viewer" object to the nodes (label, description, folder "Active",
// exception timing), which the CLI ignores.

#ifndef BONSAIVIEWER_SERVICES_CLASH_RULEJSON_H
#define BONSAIVIEWER_SERVICES_CLASH_RULEJSON_H

#include "RuleModel.h"

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>

#include <stdexcept>

namespace bonsaiviewer::services::clash::json {

// A malformed document (unknown node or facet type...).
struct FormatError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

QJsonObject toJson(const RuleFile& file);
QJsonObject toJson(const RuleFolder& folder);
QJsonObject toJson(const RuleCheck& rule);
QJsonValue toJson(const Select& select);
QJsonValue toJson(const BooleanNode* node);
QJsonObject toJson(const bonsaiviewer::ports::ifctester::Facet& facet);

RuleFile fileFromJson(const QJsonObject& json);
RuleNode nodeFromJson(const QJsonObject& json);
std::shared_ptr<RuleCheck> ruleFromJson(const QJsonObject& json);
Select selectFromJson(const QJsonValue& json);
std::shared_ptr<BooleanNode> booleanFromJson(const QJsonValue& json);
bonsaiviewer::ports::ifctester::Facet facetFromJson(const QJsonObject& json);

// The rule file text as a JSON object. Python's json.dump writes Infinity,
// -Infinity and NaN (a default volume_max is float('inf')): read as
// 1e308, -1e308 and null. Throws FormatError on a malformed document.
QJsonObject parseDocument(const QByteArray& text);

// A deep copy (through the JSON form): rules referenced by value, ids kept.
std::shared_ptr<RuleCheck> cloneRule(const RuleCheck& rule);
// A new node id (a uuid, as the Python ids).
QString newId();

} // namespace bonsaiviewer::services::clash::json

#endif // BONSAIVIEWER_SERVICES_CLASH_RULEJSON_H
