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

#ifndef IFCINTERFACE_MODULES_RULE_ENGINEPROXY_RULE_FILE_JSON_H
#define IFCINTERFACE_MODULES_RULE_ENGINEPROXY_RULE_FILE_JSON_H

#include <QJsonObject>

class QTreeWidgetItem;
namespace bonsaiviewer::modules::rule { class RuleTreeWidget; }

namespace bonsaiviewer::modules::rule::engineproxy {

// Tree -> JSON: the exact format of ifcclash_plus/serialization.py, so a
// file saved by BonsaiViewer loads in the engine's CLI and vice versa
// (integration spec section 9). The instance value maps (kRuleValuesRole)
// already use the serialization parameter names — only the facet attribute
// names are translated (predefinedType -> predefined_type, ...).

// The whole tree under a RuleFile item.
QJsonObject ruleFileToJson(QTreeWidgetItem* file_item);

// One rule or folder (folder -> {RuleFolder, contains}, rule -> the
// serialize_rule_check format with its selects and nested rules).
QJsonObject ruleItemToJson(QTreeWidgetItem* item);

// The subtree to run as a temporary RuleFile: the whole open file when
// root is null, or the given file / folder / rule item. An empty object
// when there is nothing to run (no open file, or an unknown item).
QJsonObject scopeToJson(bonsaiviewer::modules::rule::RuleTreeWidget* tree, QTreeWidgetItem* root);

// The SelectFacet JSON of a facet item (the "select" IPC request payload).
QJsonObject facetSelectToJson(QTreeWidgetItem* facet_item);

// JSON -> tree: populates `tree` (a fresh RuleFile is created, replacing
// the previous one) from a serialized RuleFile. Returns the file item or
// null on a malformed document.
QTreeWidgetItem* ruleFileFromJson(bonsaiviewer::modules::rule::RuleTreeWidget* tree,
                                  const QJsonObject& file_json);

} // namespace bonsaiviewer::modules::rule::engineproxy

#endif
