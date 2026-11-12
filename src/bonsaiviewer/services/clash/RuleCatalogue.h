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

// BonsaiViewer09 (TI09) — the rules the viewer offers: the _triang rules of
// the engine catalogue (ifcclash::catalogue_json), under their base name.

#ifndef BONSAIVIEWER_SERVICES_CLASH_RULECATALOGUE_H
#define BONSAIVIEWER_SERVICES_CLASH_RULECATALOGUE_H

#include <QJsonValue>
#include <QString>
#include <QStringList>

#include <optional>
#include <vector>

namespace bonsaiviewer::services::clash {

struct ParamSpec {
    QString name;  // the catalogue / serialization key
    QString kind;  // float, int, bool, enum, string, string_list, direction, object
    QString doc;
    QString unit;
    QJsonValue default_value;
    QStringList values; // enum
    bool nullable = false;
    std::optional<double> min;
    std::optional<double> max;
};

struct RuleSpec {
    QString type;        // the base type, the key of the rule file ("Above")
    QString engine_type; // what the engine runs ("Above_triang")
    QString label;       // "Above"
    QString family;      // OneObjectRule, TwoObjectRule, ComplexRule
    QString doc;
    QStringList slot_names; // source, target, context
    std::vector<ParamSpec> params;

    bool hasSlot(const QString& name) const { return slot_names.contains(name); }
    bool isTwoObject() const { return hasSlot("target"); }
};

class RuleCatalogue {
public:
    static const RuleCatalogue& instance();

    const std::vector<RuleSpec>& rules() const { return rules_; }
    // By base type ("Above") or engine type ("Above_triang"); null when unknown.
    const RuleSpec* find(const QString& type) const;
    QString engineVersion() const { return engine_version_; }

private:
    RuleCatalogue();
    std::vector<RuleSpec> rules_;
    QString engine_version_;
};

// "1 object", "2 objects", "complex": the type badge of the tree.
QString familyBadge(const QString& family);

} // namespace bonsaiviewer::services::clash

#endif // BONSAIVIEWER_SERVICES_CLASH_RULECATALOGUE_H
