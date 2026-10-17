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

#include "RuleFileJson.h"

#include "../Catalogue.h"
#include "../RulePanel.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QVariantMap>

namespace bonsaiviewer::modules::rule::engineproxy {

namespace {

using bonsaiviewer::modules::rule::findRuleDefinition;
using bonsaiviewer::modules::rule::initRuleInstance;
using bonsaiviewer::modules::rule::kDescriptionRole;
using bonsaiviewer::modules::rule::kNodeIdRole;
using bonsaiviewer::modules::rule::kRuleKeyRole;
using bonsaiviewer::modules::rule::kRuleValuesRole;
using bonsaiviewer::modules::rule::kSlotNameRole;
using RuleNodeType = bonsaiviewer::modules::rule::RuleNodeType;
using RulePropertyType = bonsaiviewer::modules::rule::RulePropertyType;
using RuleTreeWidget = bonsaiviewer::modules::rule::RuleTreeWidget;

QString nodeId(QTreeWidgetItem* item, const QString& fallback_key) {
    QString id = item->data(0, kNodeIdRole).toString();
    if (id.isEmpty()) {
        id = fallback_key;
        item->setData(0, kNodeIdRole, id);
    }
    return id;
}

// The facet attribute names of the catalogue (the ifctester attributes)
// translated to the serialization keys of ifcclash_plus/serialization.py.
QString facetParamKey(const QString& facet_type, const QString& param) {
    static const std::pair<const char*, const char*> mapping[] = {
        {"Entity.predefinedType", "predefined_type"},
        {"Property.propertySet", "property_set"},
        {"Property.baseName", "base_name"},
        {"Property.dataType", "data_type"},
        {"PartOf.predefinedType", "predefined_type"},
    };
    const QString qualified = facet_type + QLatin1Char('.') + param;
    for (const auto& entry : mapping) {
        if (qualified == QLatin1String(entry.first)) return QLatin1String(entry.second);
    }
    return param;
}

// A facet value: a plain string or a restriction block
// {restriction: kind, values: [...]}, the shared IDS form.
QJsonValue facetValueToJson(const QVariant& value) {
    if (value.userType() == QMetaType::QVariantMap) {
        const QVariantMap map = value.toMap();
        QJsonObject restriction;
        restriction.insert("restriction", map.value("restriction").toString());
        QJsonArray values;
        for (const QString& entry : map.value("values").toStringList()) values.append(entry);
        restriction.insert("values", values);
        return restriction;
    }
    return value.toString();
}

QJsonObject facetToJson(QTreeWidgetItem* item) {
    const QString key = item->data(0, kRuleKeyRole).toString();
    QJsonObject facet;
    facet.insert("type", key);
    const QVariantMap values = item->data(0, kRuleValuesRole).toMap();
    if (const auto* definition = findRuleDefinition(key)) {
        for (const auto& property : definition->properties) {
            const QString json_key = facetParamKey(key, property.name);
            const QVariant value = values.value(property.name);
            // Omit empty strings: the engine reads a missing key as None.
            if (value.userType() == QMetaType::QString && value.toString().isEmpty()) continue;
            facet.insert(json_key, facetValueToJson(value));
        }
    }
    return facet;
}

bool isFacetItem(QTreeWidgetItem* item) {
    const auto* definition = findRuleDefinition(item->data(0, kRuleKeyRole).toString());
    return definition && definition->family == QLatin1String("IDS Facet");
}

// A value of one of the engine's parameter kinds, as the deserializer of
// serialization.py reads it. Returns an undefined value for "absent".
QJsonValue paramToJson(const QVariant& value, RulePropertyType type) {
    switch (type) {
    case RulePropertyType::Text:
        return value.toString();
    case RulePropertyType::Float:
        return value.toDouble();
    case RulePropertyType::NullableFloat: {
        if (value.userType() == QMetaType::QString) return QJsonValue(); // empty = none
        return value.toDouble();
    }
    case RulePropertyType::Int: {
        if (value.userType() == QMetaType::QString) return QJsonValue();
        return value.toInt();
    }
    case RulePropertyType::Bool:
        return value.toBool();
    case RulePropertyType::Enum: {
        const QString text = value.toString();
        return text.isEmpty() ? QJsonValue(QJsonValue::Undefined) : QJsonValue(text);
    }
    case RulePropertyType::StringList: {
        QJsonArray array;
        for (const QString& entry : value.toStringList()) array.append(entry);
        return array;
    }
    case RulePropertyType::Direction: {
        QJsonArray array;
        for (const QVariant& axis : value.toList()) array.append(axis.toDouble());
        return array;
    }
    }
    return QJsonValue();
}

// The bare display name of an item, without any "source : " slot prefix.
QString bareLabel(QTreeWidgetItem* item) {
    const QString text = item->text(0);
    const int split = text.indexOf(QStringLiteral(" : "));
    return split >= 0 ? text.mid(split + 3) : text;
}

QJsonObject selectToJson(QTreeWidgetItem* child) {
    if (isFacetItem(child)) {
        QJsonObject select;
        select.insert("type", QStringLiteral("SelectFacet"));
        select.insert("classification_type", QStringLiteral("Facet"));
        select.insert("classification_name", QString());
        QJsonArray applicability;
        applicability.append(facetToJson(child));
        select.insert("applicability", applicability);
        return select;
    }
    // A rule dropped in a slot: a SelectRule referencing it (nested by value).
    QJsonObject select;
    select.insert("type", QStringLiteral("SelectRule"));
    select.insert("element_to_pass", QStringLiteral("source"));
    select.insert("element_state_to_pass", true);
    select.insert("rule", ruleItemToJson(child));
    return select;
}

// The select of a slot ("source" -> select_source, "context" -> select_context).
QTreeWidgetItem* slotChildOf(QTreeWidgetItem* rule_item, const QString& slot_name) {
    for (int i = 0; i < rule_item->childCount(); ++i) {
        QTreeWidgetItem* child = rule_item->child(i);
        if (child->data(0, kSlotNameRole).toString() == slot_name
            && RuleTreeWidget::nodeType(child) == RuleNodeType::Rule) {
            return child;
        }
    }
    return nullptr;
}

void insertSelects(QJsonObject& rule_json, QTreeWidgetItem* item) {
    static const std::pair<QString, QString> select_keys[] = {
        {QStringLiteral("source"), QStringLiteral("select_source")},
        {QStringLiteral("target"), QStringLiteral("select_target")},
        {QStringLiteral("context"), QStringLiteral("select_context")},
    };
    for (const auto& slot : select_keys) {
        QTreeWidgetItem* child = slotChildOf(item, slot.first);
        if (child) rule_json.insert(slot.second, selectToJson(child));
    }
}

QJsonObject viewerBlock(QTreeWidgetItem* item) {
    QJsonObject viewer;
    viewer.insert("label", bareLabel(item));
    viewer.insert("description", item->data(0, kDescriptionRole).toString());
    return viewer;
}

QJsonObject folderToJson(QTreeWidgetItem* item) {
    QJsonObject folder;
    folder.insert("type", QStringLiteral("RuleFolder"));
    folder.insert("id", nodeId(item, QStringLiteral("folder")));
    folder.insert("activation_case", QStringLiteral("ALLTRUE"));
    folder.insert("activation_rule", QJsonValue());
    QJsonArray contains;
    for (int i = 0; i < item->childCount(); ++i) {
        const QJsonObject child = ruleItemToJson(item->child(i));
        if (!child.isEmpty()) contains.append(child);
    }
    folder.insert("contains", contains);
    folder.insert("viewer", viewerBlock(item));
    return folder;
}

QVariantMap facetValuesFromJson(const QString& facet_key, const QJsonObject& json) {
    // Facets: translate the serialization keys back to the catalogue names.
    QVariantMap values;
    const auto* definition = findRuleDefinition(facet_key);
    if (!definition) return values;
    for (const auto& property : definition->properties) {
        values.insert(property.name, property.default_value);
        const QString json_key = facetParamKey(definition->name, property.name);
        if (!json.contains(json_key)) continue;
        const QJsonValue value = json.value(json_key);
        if (value.isObject()) {
            QVariantMap restriction;
            restriction.insert("restriction", value.toObject().value("restriction").toString());
            QStringList entries;
            for (const QJsonValue& entry : value.toObject().value("values").toArray()) {
                entries.append(entry.toString());
            }
            restriction.insert("values", entries);
            values.insert(property.name, restriction);
        } else {
            values.insert(property.name, value.toString());
        }
    }
    return values;
}

QVariantMap ruleValuesFromJson(const QJsonObject& json, const QString& key) {
    QVariantMap values;
    const auto* definition = findRuleDefinition(key);
    if (!definition) return values;
    for (const auto& property : definition->properties) {
        if (!json.contains(property.name)) continue;
        const QJsonValue value = json.value(property.name);
        switch (property.type) {
        case RulePropertyType::Text:
            values.insert(property.name, value.toString());
            break;
        case RulePropertyType::Float:
        case RulePropertyType::NullableFloat:
            values.insert(property.name, value.toDouble());
            break;
        case RulePropertyType::Int:
            values.insert(property.name, value.toInt());
            break;
        case RulePropertyType::Bool:
            values.insert(property.name, value.toBool());
            break;
        case RulePropertyType::Enum:
            values.insert(property.name, value.toString());
            break;
        case RulePropertyType::StringList: {
            QStringList entries;
            for (const QJsonValue& entry : value.toArray()) entries.append(entry.toString());
            values.insert(property.name, entries);
            break;
        }
        case RulePropertyType::Direction: {
            QVariantList direction;
            for (const QJsonValue& axis : value.toArray()) direction.append(axis.toDouble());
            values.insert(property.name, direction);
            break;
        }
        }
    }
    return values;
}

// Fills the slots of a freshly created rule item from its serialized selects.
void fillRuleSlotsFromJson(RuleTreeWidget* tree, QTreeWidgetItem* rule_item, const QJsonObject& json) {
    static const std::pair<QString, QString> select_keys[] = {
        {QStringLiteral("select_source"), QStringLiteral("source")},
        {QStringLiteral("select_target"), QStringLiteral("target")},
        {QStringLiteral("select_context"), QStringLiteral("context")},
    };
    for (const auto& slot : select_keys) {
        if (!json.contains(slot.first)) continue;
        const QJsonObject select = json.value(slot.first).toObject();
        // The placeholder child of this name.
        QTreeWidgetItem* placeholder = nullptr;
        for (int i = 0; i < rule_item->childCount(); ++i) {
            QTreeWidgetItem* child = rule_item->child(i);
            if (RuleTreeWidget::nodeType(child) == RuleNodeType::Slot
                && child->text(0) == slot.second) {
                placeholder = child;
                break;
            }
        }
        if (!placeholder) continue;
        if (select.value("type").toString() == QStringLiteral("SelectFacet")) {
            const QJsonArray applicability = select.value("applicability").toArray();
            if (applicability.isEmpty()) continue;
            const QJsonObject facet = applicability.at(0).toObject();
            const QString facet_key = facet.value("type").toString();
            const auto* definition = findRuleDefinition(facet_key);
            if (!definition) continue;
            tree->fillSlot(placeholder, facet_key, facetValuesFromJson(facet_key, facet),
                           facet.value("id").toString(), QString(), definition->label);
        } else if (select.value("type").toString() == QStringLiteral("SelectRule")) {
            const QJsonObject nested = select.value("rule").toObject();
            const QString nested_key = nested.value("type").toString();
            const auto* definition = findRuleDefinition(nested_key);
            if (!definition) continue;
            const QJsonObject viewer = nested.value("viewer").toObject();
            const QString label = viewer.value("label").toString(definition->label);
            QTreeWidgetItem* slot_rule = tree->fillSlot(
                placeholder, nested_key, ruleValuesFromJson(nested, nested_key),
                nested.value("id").toString(), viewer.value("description").toString(), label);
            if (slot_rule) fillRuleSlotsFromJson(tree, slot_rule, nested);
        }
    }
}

void itemFromJson(RuleTreeWidget* tree, const QJsonObject& json, QTreeWidgetItem* parent) {
    const QString type = json.value("type").toString();
    const QJsonObject viewer = json.value("viewer").toObject();
    const QString node_id = json.value("id").toString();
    if (type == QStringLiteral("RuleFolder")) {
        const QString label = viewer.value("label").toString(QStringLiteral("Folder"));
        QTreeWidgetItem* folder = tree->createFolderWithId(
            parent, label, node_id, viewer.value("description").toString());
        for (const QJsonValue& child : json.value("contains").toArray()) {
            itemFromJson(tree, child.toObject(), folder);
        }
        return;
    }
    const auto* definition = findRuleDefinition(type);
    if (!definition) return;
    const QString label = viewer.value("label").toString(definition->label);
    QTreeWidgetItem* rule = tree->addRuleInstance(
        type, ruleValuesFromJson(json, type), node_id,
        viewer.value("description").toString(), parent);
    if (rule) fillRuleSlotsFromJson(tree, rule, json);
}

} // namespace

QJsonObject ruleItemToJson(QTreeWidgetItem* item) {
    if (!item) return {};
    const RuleNodeType node = RuleTreeWidget::nodeType(item);
    if (node == RuleNodeType::Folder) return folderToJson(item);
    if (node != RuleNodeType::Rule) return {};

    const QString key = item->data(0, kRuleKeyRole).toString();
    QJsonObject rule;
    rule.insert("type", key);
    rule.insert("id", nodeId(item, QStringLiteral("rule")));
    rule.insert("state", QStringLiteral("Final"));
    rule.insert("viewer", viewerBlock(item));

    const QVariantMap values = item->data(0, kRuleValuesRole).toMap();
    if (const auto* definition = findRuleDefinition(key)) {
        for (const auto& property : definition->properties) {
            const QJsonValue value = paramToJson(values.value(property.name), property.type);
            if (!value.isUndefined()) rule.insert(property.name, value);
        }
    }
    insertSelects(rule, item);
    return rule;
}

QJsonObject ruleFileToJson(QTreeWidgetItem* file_item) {
    if (!file_item || RuleTreeWidget::nodeType(file_item) != RuleNodeType::File) return {};
    QJsonObject file;
    file.insert("type", QStringLiteral("RuleFile"));
    file.insert("id", nodeId(file_item, QStringLiteral("file")));
    file.insert("list_ifc_path", QJsonArray());
    file.insert("path_to_save", QJsonValue());
    file.insert("viewer", viewerBlock(file_item));
    QJsonArray contains;
    for (int i = 0; i < file_item->childCount(); ++i) {
        const QJsonObject child = ruleItemToJson(file_item->child(i));
        if (!child.isEmpty()) contains.append(child);
    }
    file.insert("contains", contains);
    return file;
}

QJsonObject scopeToJson(RuleTreeWidget* tree, QTreeWidgetItem* root) {
    if (!root) {
        // The whole open file.
        return ruleFileToJson(tree ? tree->openRuleFile() : nullptr);
    }
    const RuleNodeType node = RuleTreeWidget::nodeType(root);
    if (node == RuleNodeType::File) return ruleFileToJson(root);
    QJsonObject file;
    file.insert("type", QStringLiteral("RuleFile"));
    file.insert("id", QStringLiteral("scope"));
    file.insert("list_ifc_path", QJsonArray());
    file.insert("path_to_save", QJsonValue());
    QJsonArray contains;
    const QJsonObject item = ruleItemToJson(root);
    if (item.isEmpty()) return {};
    contains.append(item);
    file.insert("contains", contains);
    return file;
}

QJsonObject facetSelectToJson(QTreeWidgetItem* facet_item) {
    if (!facet_item || !isFacetItem(facet_item)) return {};
    return selectToJson(facet_item);
}

QTreeWidgetItem* ruleFileFromJson(RuleTreeWidget* tree, const QJsonObject& file_json) {
    if (file_json.value("type").toString() != QStringLiteral("RuleFile")) return nullptr;
    const QJsonObject viewer = file_json.value("viewer").toObject();
    QTreeWidgetItem* file = tree->createRuleFileWithId(
        viewer.value("label").toString(QStringLiteral("Rule File")),
        file_json.value("id").toString());
    file->setData(0, kDescriptionRole, viewer.value("description").toString());
    for (const QJsonValue& child : file_json.value("contains").toArray()) {
        itemFromJson(tree, child.toObject(), file);
    }
    return file;
}

} // namespace bonsaiviewer::modules::rule::engineproxy
