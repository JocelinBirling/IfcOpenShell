// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the License, or (at your option) any later            *
 * version.                                                                     *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_RULE_CATALOGUE_H
#define IFCINTERFACE_MODULES_RULE_CATALOGUE_H

#include <QJsonArray>
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
constexpr int kNodeIdRole = Qt::UserRole + 5;    // the engine node id (uuid) of a rule/folder/file

// What a tree node is. Stored on the item as int in Qt::UserRole.
enum class RuleNodeType {
    File,   // a RuleFile: contains one or more rules (and folders)
    Folder, // collapsible: can contain folders and 0..n rules
    Rule,   // a rule instantiated from the catalogue
    Slot,   // placeholder under a rule (source/target/context) that accepts a dropped rule
};

// The editor kinds of the engine registry (registry.py): they mirror the
// serialization parameter names of ifcclash_plus/serialization.py exactly,
// so the instance value map serializes to the engine's JSON as is.
enum class RulePropertyType {
    Text,       // a plain string (or a facet value with a Complex restriction)
    Float,      // QDoubleSpinBox
    NullableFloat, // a float or "none" (empty QLineEdit)
    Int,        // QSpinBox-like text field
    Bool,       // QCheckBox
    Enum,       // combo box (enum_values)
    StringList, // a stack of text lines (one value per line)
    Direction,  // three floats (x, y, z)
};

struct RulePropertyDef {
    QString name;
    RulePropertyType type = RulePropertyType::Text;
    QVariant default_value; // double / QString / bool / QStringList / QVariantList
    bool nullable = false;  // the engine accepts "absent" for this parameter
    QStringList enum_values;
    QString doc;
    QString unit;
};

struct RuleDefinition {
    QString name;        // the engine type (OBB_Above, Entity) — the serialization key
    QString label;       // the display name (OBB Above, Entity Facet)
    QString family;      // library section: OneObjectRule, TwoObjectRule, ComplexRule, IDS Facet
    QString description;
    std::vector<RulePropertyDef> properties;
    // Child placeholders the rule exposes once instantiated. A slot is
    // filled by an IDS facet (a SelectFacet on the engine side) or by a
    // rule (a SelectRule referencing it). ("slot_names" because "slots" is
    // a Qt macro.)
    std::vector<QString> slot_names;
};

// The rule catalogue: a static mirror of the engine registry (a startup
// fallback usable before the engine answers) replaced by the engine's own
// registry as soon as the catalogue IPC reply arrives.
const std::vector<RuleDefinition>& ruleCatalogue();
const RuleDefinition* findRuleDefinition(const QString& name);
// Resolves a dragged library text or stored key to its definition: by
// catalogue name first (the stable key), then by display label.
const RuleDefinition* resolveRuleDefinition(const QString& text);

// Replaces the catalogue with the engine registry (EngineClient's
// catalogueReady payload). Rules and facets both arrive in the same
// format; facets become family "IDS Facet" entries. Returns the number
// of definitions installed.
int setCatalogueFromEngine(const QJsonArray& rules, const QJsonArray& facets);

// Instantiates rule values on a tree item from the catalogue defaults.
void initRuleInstance(QTreeWidgetItem* item, const RuleDefinition& definition);

} // namespace bonsaiviewer::modules::rule

#endif
