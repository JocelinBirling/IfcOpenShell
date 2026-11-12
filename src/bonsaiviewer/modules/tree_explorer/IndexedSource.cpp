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
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "IndexedSource.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../services/ActionPipeline.h"
#include "../../services/ContextMenuBuilder.h"
#include "../../services/SelectionService.h"
#include "../../../ifcviewer/Federation.h"
#include "../../../ifcviewer/SceneLoader.h"

#include <QTimer>

#include <unordered_set>

namespace bonsaiviewer::modules::tree_explorer {

using bonsaiviewer::services::ActionPipeline;
using bonsaiviewer::services::tree::IndexNode;
using bonsaiviewer::services::tree::TreeIndex;
using bonsaiviewer::services::tree::TreeNode;

namespace {

// Up to this many matches, the search opens the folders down to them.
constexpr int kExpandMatchesLimit = 200;

} // namespace

IndexedSource::IndexedSource(const Context& context, QObject* parent)
    : TreeDataSource(parent), context_(context)
{
    auto* session = &context_.session;
    connect(session, &SessionState::modelGeometryReady, this, [this](uint32_t) { scheduleRebuild(); });
    connect(session, &SessionState::modelDataSourceReady, this, [this](uint32_t) { scheduleRebuild(); });
    connect(session, &SessionState::modelsChanged, this, [this]() { scheduleRebuild(); });
    connect(session, &SessionState::projectOpened, this, [this](const QString&) {
        hidden_keys_.clear();
        scheduleRebuild();
    });
    connect(session, &SessionState::projectReset, this, [this]() {
        hidden_keys_.clear();
        scheduleRebuild();
    });
    connect(session, &SessionState::activeModelChanged, this, [this](const QString&) {
        if (active_only_) scheduleRebuild();
    });
}

void IndexedSource::setActiveModelOnly(bool active_only) {
    if (active_only_ == active_only) return;
    active_only_ = active_only;
    rebuild();
}

void IndexedSource::setFilter(const QString& text) {
    filter_ = text.trimmed();
    int matches = 0;
    shown_ = index_.match(filter_, &matches);
    expand_matches_ = !filter_.isEmpty() && matches <= kExpandMatchesLimit;
    emit nodesChanged();
}

void IndexedSource::scheduleRebuild() {
    if (rebuild_pending_) return;
    rebuild_pending_ = true;
    QTimer::singleShot(0, this, [this]() {
        if (rebuild_pending_) rebuild();
    });
}

void IndexedSource::rebuild() {
    reindex();
    emit nodesChanged();
}

void IndexedSource::reindex() {
    rebuild_pending_ = false;
    TreeIndex index;
    const QList<std::uint32_t> models = modelsInScope();
    build(index, models);
    if (index.size() == 0) {
        index.add(-1, messageNode(models.isEmpty() ? QStringLiteral("No model is open.") : emptyText()));
    }
    index.finish(sorted());
    index_ = std::move(index);
    int matches = 0;
    shown_ = index_.match(filter_, &matches);
    expand_matches_ = !filter_.isEmpty() && matches <= kExpandMatchesLimit;
}

QList<std::uint32_t> IndexedSource::modelsInScope() const {
    if (!active_only_ || !supportsScope()) return context_.session.sessionModelIds();
    const QString active = context_.session.activeModelId();
    if (active.isEmpty()) return {};
    const std::uint32_t id = context_.session.sessionModelIdForModelId(active);
    return id != 0 ? QList<std::uint32_t>{id} : QList<std::uint32_t>{};
}

QString IndexedSource::modelName(std::uint32_t session_model_id) const {
    const QString model_id = context_.session.modelIdForSessionModelId(session_model_id);
    if (const auto* model = context_.session.federation()->findById(model_id);
        model && !model->display_name.isEmpty()) {
        return model->display_name;
    }
    auto* loader = context_.session.loader();
    return loader ? loader->displayName(session_model_id) : QString();
}

IndexNode IndexedSource::modelNode(std::uint32_t session_model_id, const QString& label) {
    IndexNode node;
    node.kind = IndexNode::Kind::Folder;
    node.id = QStringLiteral("model%1").arg(session_model_id);
    node.label = label;
    node.icon = QStringLiteral(":/icons/cube.svg");
    node.model = session_model_id;
    return node;
}

IndexNode IndexedSource::messageNode(const QString& label) {
    IndexNode node;
    node.kind = IndexNode::Kind::Message;
    node.label = label;
    node.tooltip = label;
    return node;
}

bool IndexedSource::isHidden(int index) const {
    for (int up = index; up >= 0; up = index_.node(up).parent) {
        if (hidden_keys_.contains(index_.node(up).key)) return true;
    }
    return false;
}

TreeNode IndexedSource::toNode(int index) const {
    const IndexNode& source = index_.node(index);
    TreeNode node;
    node.key = source.key;
    node.label = index_.displayLabel(index);
    node.icon = source.icon;
    node.tooltip = source.tooltip;
    node.is_folder = !source.children.empty();
    node.accepts_children = false;
    node.draggable = false;
    node.has_visibility = source.kind != IndexNode::Kind::Message;
    node.visibility = !isHidden(index);
    node.expanded = source.expanded || (expand_matches_ && node.is_folder);
    return node;
}

std::vector<TreeNode> IndexedSource::nodesOf(const std::vector<int>& indices) const {
    std::vector<TreeNode> nodes;
    nodes.reserve(indices.size());
    for (int index : indices) {
        if (shown_.empty() || shown_[index]) nodes.push_back(toNode(index));
    }
    return nodes;
}

std::vector<TreeNode> IndexedSource::topLevelNodes() {
    // A rebuild is due: do it now, the panel is reading the tree anyway.
    if (rebuild_pending_) reindex();
    return nodesOf(index_.roots());
}

std::vector<TreeNode> IndexedSource::childrenOf(const QString& key) {
    const int index = index_.find(key);
    if (index < 0) return {};
    return nodesOf(index_.node(index).children);
}

std::vector<std::uint32_t> IndexedSource::objectIdsOf(const QStringList& keys) const {
    std::vector<std::uint32_t> ids;
    std::unordered_set<std::uint32_t> seen;
    auto add = [&](std::uint32_t id) {
        if (id != 0 && seen.insert(id).second) ids.push_back(id);
    };
    for (const QString& key : keys) {
        const int start = index_.find(key);
        if (start < 0) continue;
        std::vector<int> stack{start};
        while (!stack.empty()) {
            const IndexNode& node = index_.node(stack.back());
            stack.pop_back();
            if (node.model != 0 && node.ifc_id == 0 && node.object_id == 0) {
                // A whole model: every object of it.
                for (const auto& info : context_.session.elementRegistry()->basicElementInfoForModel(node.model)) {
                    add(info.object_id);
                }
                continue;
            }
            add(node.object_id);
            stack.insert(stack.end(), node.children.begin(), node.children.end());
        }
    }
    return ids;
}

void IndexedSource::onSelectionChanged(const QStringList& keys) {
    selected_keys_ = keys;
    const auto ids = objectIdsOf(keys);
    if (!ids.empty()) context_.selection.setSelection(ids, bonsaiviewer::services::SelectionSource::Tree);
}

void IndexedSource::populateContextMenu(const QString& key, bonsaiviewer::services::ContextMenuBuilder& menu) {
    const int index = index_.find(key);
    if (index < 0 || index_.node(index).kind == IndexNode::Kind::Message) return;
    // On a selected node, the actions apply to the whole selection.
    const QStringList keys = selected_keys_.contains(key) ? selected_keys_ : QStringList{key};
    menu.addItem("isolate", "Isolate", [this, keys]() {
        context_.actions.run(objectIdsOf(keys), ActionPipeline::Action::Isolate);
    });
    menu.addItem("select", "Select", [this, keys]() {
        context_.selection.setSelection(objectIdsOf(keys), bonsaiviewer::services::SelectionSource::Tree);
    });
    menu.addItem("hide", "Hide", [this, keys]() {
        for (const QString& k : keys) onVisibilityToggled(k, false);
    });
    menu.addItem("show", "Show", [this, keys]() {
        for (const QString& k : keys) onVisibilityToggled(k, true);
    });
    const IndexNode& node = index_.node(index);
    if (node.model != 0 && node.ifc_id != 0) {
        menu.addSeparator();
        const std::uint32_t model = node.model;
        const int ifc_id = node.ifc_id;
        menu.addItem("properties", "Show properties", [this, model, ifc_id]() {
            context_.session.inspectEntity(model, ifc_id);
        });
    }
}

void IndexedSource::onNodeDoubleClicked(const QString& key) {
    const auto ids = objectIdsOf({key});
    if (!ids.empty()) context_.actions.run(ids, ActionPipeline::Action::Isolate);
}

void IndexedSource::setHidden(int index, bool hidden) {
    const QString& key = index_.node(index).key;
    if (hidden) {
        hidden_keys_.insert(key);
        return;
    }
    // Showing a node shows everything below it.
    for (auto it = hidden_keys_.begin(); it != hidden_keys_.end();) {
        const bool below = *it == key || it->startsWith(key + QLatin1Char('/'));
        it = below ? hidden_keys_.erase(it) : std::next(it);
    }
    // Under a hidden parent, the node alone comes back: the parent's other
    // children stay hidden, at every level down from the highest hidden one.
    std::vector<int> path;  // the node and its ancestors, bottom up
    int highest_hidden = -1;
    for (int up = index; up >= 0; up = index_.node(up).parent) {
        path.push_back(up);
        if (up != index && hidden_keys_.contains(index_.node(up).key)) highest_hidden = static_cast<int>(path.size()) - 1;
    }
    for (int level = highest_hidden; level > 0; --level) {
        const IndexNode& parent = index_.node(path[level]);
        hidden_keys_.remove(parent.key);
        for (int child : parent.children) {
            if (child != path[level - 1]) hidden_keys_.insert(index_.node(child).key);
        }
    }
}

void IndexedSource::onVisibilityToggled(const QString& key, bool visible) {
    const int index = index_.find(key);
    if (index < 0) return;
    setHidden(index, !visible);
    const auto ids = objectIdsOf({key});
    if (!ids.empty()) {
        context_.actions.run(ids, visible ? ActionPipeline::Action::AddToView : ActionPipeline::Action::RemoveFromView);
    }
    // The eyes below change too; refresh after the click has been handled.
    QMetaObject::invokeMethod(this, [this]() { emit nodesChanged(); }, Qt::QueuedConnection);
}

} // namespace bonsaiviewer::modules::tree_explorer
