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

#ifndef IFCINTERFACE_MODULES_FILTERS_VIEW_H
#define IFCINTERFACE_MODULES_FILTERS_VIEW_H

#include "CreationDialog.h"

#include "../../services/FilterStore.h"
#include "../../services/TreePanelFramework.h"

#include <QObject>
#include <QString>
#include <vector>

namespace bonsaiviewer {
class SessionState;
}

namespace bonsaiviewer::services {
class FilterRunner;
class IdsBridge;
class ActionPipeline;
}

namespace bonsaiviewer::modules::filters {

class FiltersPanel;

// B06 — the Filters panel view: the B12 tree data source (folders and
// filters, drag and drop, context menus) plus the flows around it —
// create / modify through the Filter Creation Window, run / run on the
// box through FilterRunner, import / export envelopes.
class FiltersPanelView : public bonsaiviewer::services::tree::TreeDataSource {
    Q_OBJECT

public:
    FiltersPanelView(FiltersPanel* panel,
                     bonsaiviewer::SessionState& session_state,
                     bonsaiviewer::services::FilterStore& store,
                     bonsaiviewer::services::FilterRunner& runner,
                     bonsaiviewer::services::IdsBridge& ids_bridge,
                     QObject* parent = nullptr);

    // The creation window's preview isolates its elements through B08.
    void setActionPipeline(bonsaiviewer::services::ActionPipeline* action_pipeline);

    // tree::TreeDataSource
    std::vector<bonsaiviewer::services::tree::TreeNode> topLevelNodes() override;
    std::vector<bonsaiviewer::services::tree::TreeNode> childrenOf(const QString& key) override;
    void populateContextMenu(const QString& key,
                             bonsaiviewer::services::ContextMenuBuilder& menu) override;
    void onVisibilityToggled(const QString& key, bool visible) override;
    // Double-click on a filter runs it on the whole model.
    void onNodeDoubleClicked(const QString& key) override;
    // Drag and drop: the store owns the tree, the widget only mirrors it.
    void onNodeMoved(const QString& key, const QString& new_parent) override;

private:
    void addFolder(const QString& parent_id);
    void addFilter(const QString& folder_id);
    void modifyFilter(const QString& filter_id);
    void runFilter(const QString& filter_id, bool on_box);
    void deleteFilter(const QString& filter_id);
    // A copy "<name> (copy)" in the same folder.
    void duplicateFilter(const QString& filter_id);
    // BonsaiViewer06 Activate / Deactivate: forces the filter's colours.
    void setFilterActive(const QString& filter_id, bool active);
    void renameFolder(const QString& folder_id);
    void deleteFolder(const QString& folder_id);
    void exportFilter(const QString& filter_id);
    void importFilter(const QString& folder_id);
    void exportLibrary();
    // Opens the creation window once every layer of `filter_id` has been
    // parsed back into facet fields (Modify), or immediately (New).
    void openCreationDialog(const bonsaiviewer::services::FilterDocument& document,
                            const std::vector<FilterCreationDialog::EditLayer>& layers,
                            const QString& folder_id);

    FiltersPanel* panel_ = nullptr;
    bonsaiviewer::SessionState& session_state_;
    bonsaiviewer::services::FilterStore& store_;
    bonsaiviewer::services::FilterRunner& runner_;
    bonsaiviewer::services::IdsBridge& ids_bridge_;
    bonsaiviewer::services::tree::TreePanelWidget* tree_panel_ = nullptr;
    bonsaiviewer::services::ActionPipeline* action_pipeline_ = nullptr;
};

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_VIEW_H
