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

#include "Catalogue.h"

#include <QTreeWidgetItem>

namespace bonsaiviewer::modules::rule {

const std::vector<RuleDefinition>& ruleCatalogue() {
    // Slot map by rule family: OneObjectRule works on one object (source),
    // TwoObjectRule relates two objects (source + target), ComplexRule also
    // knows about the surrounding model (context). IDS requirement facets
    // apply to a single data set (source); the Entity facet selects what the
    // specification applies to (context).
    static const std::vector<QString> one_object_slots = {"source"};
    static const std::vector<QString> two_object_slots = {"source", "target"};
    static const std::vector<QString> complex_slots = {"source", "target", "context"};
    static const std::vector<QString> requirement_facet_slots = {"source"};
    static const std::vector<QString> applicability_facet_slots = {"context"};

    static const std::vector<RuleDefinition> catalogue = {
        {"Name Contains", "OneObjectRule",
         "Matches objects whose name contains the given text.",
         {{"Search Text", RulePropertyType::Text, QString("")},
          {"Case Sensitive", RulePropertyType::Bool, false}},
         one_object_slots},
        {"Minimum Volume", "OneObjectRule",
         "Flags objects whose volume is below the threshold.",
         {{"Threshold", RulePropertyType::Float, 1.0},
          {"Unit", RulePropertyType::Text, QString("m3")}},
         one_object_slots},
        {"Maximum Height", "OneObjectRule",
         "Flags objects taller than the configured limit.",
         {{"Limit", RulePropertyType::Float, 3.0},
          {"Unit", RulePropertyType::Text, QString("m")},
          {"Warn Only", RulePropertyType::Bool, false}},
         one_object_slots},
        {"Minimum Distance", "TwoObjectRule",
         "Checks pairs of objects that sit closer than the threshold.",
         {{"Distance", RulePropertyType::Float, 0.5},
          {"Unit", RulePropertyType::Text, QString("m")}},
         two_object_slots},
        {"Overlap Check", "TwoObjectRule",
         "Detects pairs of objects whose volumes intersect.",
         {{"Tolerance", RulePropertyType::Float, 0.001},
          {"Report First Only", RulePropertyType::Bool, true}},
         two_object_slots},
        {"Rule Chain", "ComplexRule",
         "Applies a sequence of rules, passing matches to the next step.",
         {{"Separator", RulePropertyType::Text, QString("/")},
          {"Steps", RulePropertyType::Float, 2.0}},
         complex_slots},
        {"Group Filter", "ComplexRule",
         "Restricts rules to the objects of one group.",
         {{"Group Name", RulePropertyType::Text, QString("")},
          {"Negate", RulePropertyType::Bool, false}},
         complex_slots},
        // IDS facets: parameters mirror src/ifctester/ifctester/facet.py,
        // the project's implementation of the IDS specification.
        {"Entity Facet", "IDS Facet",
         "Shall be {name} data of type {predefinedType}.",
         {{"Name", RulePropertyType::Text, QString("IFCWALL")},
          {"Predefined Type", RulePropertyType::Text, QString("")},
          {"Instructions", RulePropertyType::Text, QString("")}},
         applicability_facet_slots},
        {"Attribute Facet", "IDS Facet",
         "The {name} of an element shall be {value}.",
         {{"Name", RulePropertyType::Text, QString("Name")},
          {"Value", RulePropertyType::Text, QString("")},
          {"Cardinality", RulePropertyType::Text, QString("required")},
          {"Instructions", RulePropertyType::Text, QString("")}},
         requirement_facet_slots},
        {"Property Facet", "IDS Facet",
         "Shall have a property set {propertySet} with a {baseName} of {value}.",
         {{"Property Set", RulePropertyType::Text, QString("Property_Set")},
          {"Base Name", RulePropertyType::Text, QString("PropertyName")},
          {"Value", RulePropertyType::Text, QString("")},
          {"Data Type", RulePropertyType::Text, QString("")},
          {"Cardinality", RulePropertyType::Text, QString("required")}},
         requirement_facet_slots},
        {"Classification Facet", "IDS Facet",
         "Shall have a {system} reference of {value}.",
         {{"Value", RulePropertyType::Text, QString("")},
          {"System", RulePropertyType::Text, QString("")},
          {"Cardinality", RulePropertyType::Text, QString("required")}},
         requirement_facet_slots},
        {"Material Facet", "IDS Facet",
         "Shall have a material of {value}.",
         {{"Value", RulePropertyType::Text, QString("")},
          {"Cardinality", RulePropertyType::Text, QString("required")}},
         requirement_facet_slots},
    };
    return catalogue;
}

const RuleDefinition* findRuleDefinition(const QString& name) {
    for (const auto& definition : ruleCatalogue()) {
        if (definition.name == name) return &definition;
    }
    return nullptr;
}

void initRuleInstance(QTreeWidgetItem* item, const RuleDefinition& definition) {
    QVariantMap values;
    for (const auto& property : definition.properties) {
        values.insert(property.name, property.default_value);
    }
    item->setData(0, kRuleKeyRole, definition.name);
    item->setData(0, kRuleValuesRole, values);
    item->setData(0, kDescriptionRole, definition.description);
}

} // namespace bonsaiviewer::modules::rule
