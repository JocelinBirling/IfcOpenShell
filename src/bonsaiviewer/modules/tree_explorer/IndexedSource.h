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

#ifndef IFCINTERFACE_MODULES_TREE_EXPLORER_INDEXEDSOURCE_H
#define IFCINTERFACE_MODULES_TREE_EXPLORER_INDEXEDSOURCE_H

#include "../../services/TreeIndex.h"
#include "../../services/TreePanelFramework.h"

#include <QList>
#include <QSet>
#include <QStringList>

#include <cstdint>
#include <vector>

namespace bonsaiviewer {
class SessionState;
}
namespace bonsaiviewer::services {
class ActionPipeline;
class SelectionService;
}

namespace bonsaiviewer::modules::tree_explorer {

// The services every tree uses.
struct Context {
    bonsaiviewer::SessionState& session;
    bonsaiviewer::services::SelectionService& selection;
    bonsaiviewer::services::ActionPipeline& actions;
};

// BonsaiViewer32 — the base of the Tree Explorer data sources. A tree only
// builds its TreeIndex from the open models; this class gives every tree
// the same interactions: click selects the elements below the selected
// nodes, double-click isolates, the eye hides / shows, the right-click menu
// (Isolate, Select, Hide, Show, Show properties) and the search.
// A node with a model and no IFC id nor object id stands for the whole model.
class IndexedSource : public bonsaiviewer::services::tree::TreeDataSource {
    Q_OBJECT

public:
    explicit IndexedSource(const Context& context, QObject* parent = nullptr);

    // Trees deduced by a computation, not read from the models.
    virtual bool interpreted() const { return false; }
    // The message of an empty tree when models are open.
    virtual QString emptyText() const { return QStringLiteral("Nothing to show."); }

    // Filters the nodes by name; the parents of a match stay shown.
    void setFilter(const QString& text);
    // Rebuilds on the next turn of the event loop (several events coalesce).
    void scheduleRebuild();
    void rebuild();

    // tree::TreeDataSource
    std::vector<bonsaiviewer::services::tree::TreeNode> topLevelNodes() override;
    std::vector<bonsaiviewer::services::tree::TreeNode> childrenOf(const QString& key) override;
    void populateContextMenu(const QString& key, bonsaiviewer::services::ContextMenuBuilder& menu) override;
    void onNodeDoubleClicked(const QString& key) override;
    void onVisibilityToggled(const QString& key, bool visible) override;

    // The tree's selection changed (Ctrl / Shift for several nodes).
    void onSelectionChanged(const QStringList& keys);

protected:
    // Fills `index` (finish() is called afterwards) for `models`.
    virtual void build(bonsaiviewer::services::tree::TreeIndex& index, const QList<std::uint32_t>& models) = 0;

    QString modelName(std::uint32_t session_model_id) const;
    // A node standing for a whole model (Models and Spatial trees).
    static bonsaiviewer::services::tree::IndexNode modelNode(std::uint32_t session_model_id, const QString& label);
    static bonsaiviewer::services::tree::IndexNode messageNode(const QString& label);

    Context context_;

private:
    void reindex();
    bonsaiviewer::services::tree::TreeNode toNode(int index) const;
    std::vector<bonsaiviewer::services::tree::TreeNode> nodesOf(const std::vector<int>& indices) const;
    std::vector<std::uint32_t> objectIdsOf(const QStringList& keys) const;
    bool isHidden(int index) const;
    void setHidden(int index, bool hidden);

    bonsaiviewer::services::tree::TreeIndex index_;
    std::vector<char> shown_;
    QString filter_;
    bool expand_matches_ = false;
    bool rebuild_pending_ = false;
    QStringList selected_keys_;
    // The keys of the nodes hidden with the eye; kept across rebuilds.
    QSet<QString> hidden_keys_;
};

} // namespace bonsaiviewer::modules::tree_explorer

#endif
