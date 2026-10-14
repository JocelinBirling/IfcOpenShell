// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or           *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                     *
 * but WITHOUT ANY WARRANTY; without the implied warranty of                     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_RULE_CATALOGUE_H
#define IFCINTERFACE_MODULES_RULE_CATALOGUE_H

#include <QString>
#include <QVariant>
#include <vector>

class QTreeWidgetItem;

namespace bonsaiviewer::modules::rule {

// QTreeWidgetItem data roles shared by the Rule tree and the Rule Config panel.
constexpr int kRuleKeyRole = Qt::UserRole + 1;    // catalogue name of a rule instance
constexpr int kRuleValuesRole = Qt::UserRole + 2; // QVariantMap of property values
constexpr int kDescriptionRole = Qt::UserRole + 3;
constexpr int kSlotNameRole = Qt::UserRole + 4;  // role label ("source"/"target"/"context") a rule was dropped into

// What a tree node is. Stored on the item as int in Qt::UserRole.
enum class RuleNodeType {
    File,   // a RuleFile: contains one or more rules (and folders)
    Folder, // collapsible: can contain folders and 0..n rules
    Rule,   // a rule instantiated from the catalogue
    Slot,   // placeholder under a rule (source/target/context) that accepts a dropped rule
};

enum class RulePropertyType {
    Text,
    Float,
    Bool,
};

struct RulePropertyDef {
    QString name;
    RulePropertyType type;
    QVariant default_value;
};

struct RuleDefinition {
    QString name;
    QString family;      // library section: OneObjectRule, TwoObjectRule, ...
    QString description;
    std::vector<RulePropertyDef> properties;
    // Child placeholders the rule exposes once instantiated; other rules are
    // dragged from the library into these slots, recursively. ("slot_names"
    // because "slots" is a Qt macro.)
    std::vector<QString> slot_names;
};

// The invented rule library. Each rule carries its own property set.
const std::vector<RuleDefinition>& ruleCatalogue();
const RuleDefinition* findRuleDefinition(const QString& name);

// Instantiates rule values on a tree item from the catalogue defaults.
void initRuleInstance(QTreeWidgetItem* item, const RuleDefinition& definition);

} // namespace bonsaiviewer::modules::rule

#endif
