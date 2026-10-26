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

#include "ClassTreeDataSource.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../services/ActionPipeline.h"
#include "../../services/ContextMenuBuilder.h"
#include "../../services/SelectionService.h"

#include <QStringList>

#include <algorithm>
#include <iterator>
#include <utility>

namespace bonsaiviewer::modules::class_tree {

using bonsaiviewer::services::tree::TreeNode;

namespace {

// Node keys: a class node is "class/<IfcClass>", an instance node is its object id.
const QString kClassPrefix = QStringLiteral("class/");

bool isClassKey(const QString& key) {
    return key.startsWith(kClassPrefix);
}

QString classOf(const QString& key) {
    return key.mid(kClassPrefix.size());
}

bool lessByLabel(const QString& a, const QString& b) {
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

TreeNode makeNode(const QString& key, const QString& label, bool is_folder) {
    TreeNode node;
    node.key = key;
    node.label = label;
    node.is_folder = is_folder;
    node.accepts_children = false;
    node.draggable = false;
    return node;
}

} // namespace

ClassTreeDataSource::ClassTreeDataSource(bonsaiviewer::SessionState& session,
                                         bonsaiviewer::services::SelectionService& selection,
                                         bonsaiviewer::services::ActionPipeline& actions,
                                         QObject* parent)
    : TreeDataSource(parent), session_(session), selection_(selection), actions_(actions)
{
    // A model event rebuilds that model's classes only.
    connect(&session_, &bonsaiviewer::SessionState::modelGeometryReady, this, [this](uint32_t id) {
        rebuildModel(id);
        emit nodesChanged();
    });
    connect(&session_, &bonsaiviewer::SessionState::modelDataSourceReady, this, [this](uint32_t id) {
        rebuildModel(id);
        emit nodesChanged();
    });
    connect(&session_, &bonsaiviewer::SessionState::projectOpened, this, [this](const QString&) {
        rebuildAll();
        emit nodesChanged();
    });
    connect(&session_, &bonsaiviewer::SessionState::projectReset, this, [this]() {
        rebuildAll();
        emit nodesChanged();
    });
    rebuildAll();
}

void ClassTreeDataSource::rebuildModel(std::uint32_t session_model_id) {
    ClassIndex index;
    for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
        if (info.type.isEmpty()) continue;
        // Instances without a name show their GlobalId.
        const QString label = info.name.trimmed().isEmpty() ? info.guid : info.name;
        index[info.type].push_back(Instance{info.object_id, label});
    }
    models_[session_model_id] = std::move(index);

    // Forget the models that were closed.
    const auto open = session_.sessionModelIds();
    for (auto it = models_.begin(); it != models_.end();) {
        const bool is_open = std::find(open.begin(), open.end(), it.key()) != open.end();
        it = is_open ? std::next(it) : models_.erase(it);
    }
}

void ClassTreeDataSource::rebuildAll() {
    models_.clear();
    for (std::uint32_t session_model_id : session_.sessionModelIds()) {
        rebuildModel(session_model_id);
    }
}

std::vector<std::uint32_t> ClassTreeDataSource::idsOfClass(const QString& ifc_class) const {
    std::vector<std::uint32_t> ids;
    for (const ClassIndex& index : models_) {
        const auto found = index.constFind(ifc_class);
        if (found == index.cend()) continue;
        for (const Instance& instance : found.value()) ids.push_back(instance.object_id);
    }
    return ids;
}

std::vector<std::uint32_t> ClassTreeDataSource::idsOfKeys(const QStringList& keys) const {
    std::vector<std::uint32_t> ids;
    for (const QString& key : keys) {
        if (isClassKey(key)) {
            const auto class_ids = idsOfClass(classOf(key));
            ids.insert(ids.end(), class_ids.begin(), class_ids.end());
        } else {
            ids.push_back(key.toUInt());
        }
    }
    return ids;
}

void ClassTreeDataSource::onSelectionChanged(const QStringList& keys) {
    selected_keys_ = keys;
    selection_.setSelection(idsOfKeys(keys), bonsaiviewer::services::SelectionSource::Tree);
}

std::vector<TreeNode> ClassTreeDataSource::topLevelNodes() {
    // Count the instances of each class over the models.
    QMap<QString, int> counts;
    for (const ClassIndex& index : models_) {
        for (auto it = index.cbegin(); it != index.cend(); ++it) {
            counts[it.key()] += static_cast<int>(it.value().size());
        }
    }
    QStringList classes = counts.keys();
    std::sort(classes.begin(), classes.end(), lessByLabel);

    std::vector<TreeNode> nodes;
    nodes.reserve(classes.size());
    for (const QString& ifc_class : classes) {
        nodes.push_back(makeNode(kClassPrefix + ifc_class,
                                 QStringLiteral("%1 (%2)").arg(ifc_class).arg(counts.value(ifc_class)),
                                 true));
    }
    return nodes;
}

std::vector<TreeNode> ClassTreeDataSource::childrenOf(const QString& key) {
    if (!isClassKey(key)) return {};
    std::vector<std::pair<QString, std::uint32_t>> instances;
    const QString ifc_class = classOf(key);
    for (const ClassIndex& index : models_) {
        const auto found = index.constFind(ifc_class);
        if (found == index.cend()) continue;
        for (const Instance& instance : found.value()) {
            instances.emplace_back(instance.label, instance.object_id);
        }
    }
    std::sort(instances.begin(), instances.end(), [](const auto& a, const auto& b) {
        return lessByLabel(a.first, b.first);
    });

    std::vector<TreeNode> nodes;
    nodes.reserve(instances.size());
    for (const auto& [label, object_id] : instances) {
        nodes.push_back(makeNode(QString::number(object_id), label, false));
    }
    return nodes;
}

void ClassTreeDataSource::populateContextMenu(const QString& key,
                                              bonsaiviewer::services::ContextMenuBuilder& menu) {
    if (key.isEmpty()) return;
    // On a selected node, Isolate applies to the whole selection.
    const QStringList keys = selected_keys_.contains(key) ? selected_keys_ : QStringList{key};
    menu.addItem({.id = "isolate", .label = "Isolate",
                  .trigger = [this, keys]() {
                      actions_.run(idsOfKeys(keys),
                                   bonsaiviewer::services::ActionPipeline::Action::Isolate);
                  }});
}

void ClassTreeDataSource::onNodeDoubleClicked(const QString& key) {
    if (key.isEmpty() || isClassKey(key)) return;
    actions_.run({key.toUInt()}, bonsaiviewer::services::ActionPipeline::Action::Isolate);
}

void ClassTreeDataSource::onVisibilityToggled(const QString&, bool) {
    // The class tree has no visibility column.
}

} // namespace bonsaiviewer::modules::class_tree
