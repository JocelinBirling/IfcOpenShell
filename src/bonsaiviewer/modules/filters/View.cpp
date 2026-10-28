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

#include "View.h"

#include "Panel.h"
#include "CreationDialog.h"

#include "../../SessionState.h"
#include "../../services/ContextMenuBuilder.h"
#include "../../services/FilterRunner.h"
#include "../../services/IdsBridge.h"
#include "../../services/PopupService.h"
#include "../../services/ActionPipeline.h"
#include "../../ElementRegistry.h"

#include <QHash>

#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QTimer>
#include <QTreeWidget>

#include <memory>

namespace bonsaiviewer::modules::filters {

using bonsaiviewer::services::ContextMenuBuilder;
using bonsaiviewer::services::FilterDocument;
using bonsaiviewer::services::tree::TreeNode;

namespace {

TreeNode folderNode(const QString& id, const QString& name) {
    TreeNode node;
    node.key = id;
    node.label = name;
    node.icon = ":/icons/folder.svg";
    node.is_folder = true;
    node.accepts_children = true;
    return node;
}

TreeNode filterNode(const QString& id, const QString& name) {
    TreeNode node;
    node.key = id;
    node.label = name;
    node.icon = ":/icons/filter.svg";
    return node;
}

} // namespace

FiltersPanelView::FiltersPanelView(FiltersPanel* panel,
                                   bonsaiviewer::SessionState& session_state,
                                   bonsaiviewer::services::FilterStore& store,
                                   bonsaiviewer::services::FilterRunner& runner,
                                   bonsaiviewer::services::IdsBridge& ids_bridge,
                                   QObject* parent)
    : TreeDataSource(parent),
      panel_(panel),
      session_state_(session_state),
      store_(store),
      runner_(runner),
      ids_bridge_(ids_bridge)
{
    tree_panel_ = new bonsaiviewer::services::tree::TreePanelWidget(
        panel_->treeWidget(), this, this);
    connect(&store_, &bonsaiviewer::services::FilterStore::changed,
            this, &FiltersPanelView::nodesChanged);
    connect(&runner_, &bonsaiviewer::services::FilterRunner::progress, this,
            [this](const QString& stage) { session_state_.setStatusMessage("Filters", stage); });
    connect(&runner_, &bonsaiviewer::services::FilterRunner::finished, this,
            [this](bool ok, const QString& summary) {
        session_state_.setStatusMessage("Filters", ok ? summary : "Filter failed");
        if (!ok) {
            services::popup::information(panel_, summary);
        }
    });

    // Toolbar: creation goes into the selected folder (or the folder of
    // the selected filter); the run buttons need a selected filter.
    const auto target_folder = [this]() -> QString {
        const QString key = panel_->currentKey();
        if (key.isEmpty()) return {};
        if (store_.isFolder(key)) return key;
        const auto document = store_.filter(key);
        return document ? document->folder_id : QString();
    };
    const auto selected_filter = [this]() -> QString {
        const QString key = panel_->currentKey();
        return (!key.isEmpty() && store_.filter(key)) ? key : QString();
    };
    const auto update_run_buttons = [this, selected_filter]() {
        panel_->setRunEnabled(!selected_filter().isEmpty() && !runner_.isRunning());
    };
    const auto update_empty_hint = [this]() {
        panel_->setEmptyHintVisible(store_.folders().empty() && store_.filtersInFolder({}).empty());
    };
    connect(panel_, &FiltersPanel::addFilterRequested, this,
            [this, target_folder]() { addFilter(target_folder()); });
    connect(panel_, &FiltersPanel::addFolderRequested, this,
            [this, target_folder]() { addFolder(target_folder()); });
    connect(panel_, &FiltersPanel::runRequested, this, [this, selected_filter]() {
        const QString key = selected_filter();
        if (!key.isEmpty() && !runner_.isRunning()) runFilter(key, false);
    });
    connect(panel_, &FiltersPanel::runOnBoxRequested, this, [this, selected_filter]() {
        const QString key = selected_filter();
        if (!key.isEmpty() && !runner_.isRunning()) runFilter(key, true);
    });
    connect(panel_, &FiltersPanel::importRequested, this,
            [this, target_folder]() { importFilter(target_folder()); });
    connect(panel_, &FiltersPanel::exportRequested, this, [this]() { exportLibrary(); });
    connect(panel_->treeWidget(), &QTreeWidget::currentItemChanged, this,
            [update_run_buttons](QTreeWidgetItem*, QTreeWidgetItem*) { update_run_buttons(); });
    connect(&runner_, &bonsaiviewer::services::FilterRunner::finished, this,
            [update_run_buttons](bool, const QString&) { update_run_buttons(); });
    connect(&store_, &bonsaiviewer::services::FilterStore::changed, this,
            [update_empty_hint, update_run_buttons]() {
        update_empty_hint();
        update_run_buttons();
    });
    update_empty_hint();

    emit nodesChanged();
    // First display: the top-level folders are open, so the examples show.
    QTimer::singleShot(0, this, [this] {
        QTreeWidget* tree = panel_->treeWidget();
        for (int i = 0; i < tree->topLevelItemCount(); ++i) tree->topLevelItem(i)->setExpanded(true);
    });
}

std::vector<TreeNode> FiltersPanelView::topLevelNodes() {
    std::vector<TreeNode> nodes;
    for (const auto& entry : store_.folders()) {
        if (entry.parent_id.isEmpty()) {
            nodes.push_back(folderNode(entry.id, entry.name));
        }
    }
    for (const auto& document : store_.filtersInFolder({})) {
        nodes.push_back(filterNode(document.id, document.name));
    }
    return nodes;
}

std::vector<TreeNode> FiltersPanelView::childrenOf(const QString& key) {
    std::vector<TreeNode> nodes;
    for (const auto& entry : store_.folders()) {
        if (entry.parent_id == key) {
            nodes.push_back(folderNode(entry.id, entry.name));
        }
    }
    for (const auto& document : store_.filtersInFolder(key)) {
        nodes.push_back(filterNode(document.id, document.name));
    }
    return nodes;
}

void FiltersPanelView::populateContextMenu(const QString& key, ContextMenuBuilder& menu) {
    if (key.isEmpty()) {
        menu.addItem("add-folder",
                     "Add Folder",
                     [this]() { addFolder({}); });
        menu.addItem("add-filter",
                     "Add Filter",
                     [this]() { addFilter({}); });
        menu.addSeparator();
        menu.addItem("import",
                     "Import Filters…",
                     [this]() { importFilter({}); });
        menu.addItem("export-all",
                     "Export All Filters…",
                     [this]() { exportLibrary(); },
                     [this]() { return !store_.folders().empty() || !store_.filtersInFolder({}).empty(); });
        return;
    }
    if (store_.isFolder(key)) {
        menu.addItem("add-folder",
                     "Add Folder",
                     [this, key]() { addFolder(key); });
        menu.addItem("add-filter",
                     "Add Filter",
                     [this, key]() { addFilter(key); });
        menu.addSeparator();
        menu.addItem("rename",
                     "Rename…",
                     [this, key]() { renameFolder(key); });
        menu.addItem("delete",
                     "Delete",
                     [this, key]() { deleteFolder(key); });
        menu.addSeparator();
        menu.addItem("import",
                     "Import Filters Here…",
                     [this, key]() { importFilter(key); });
        return;
    }
    // A filter node.
    const auto not_running = [this]() { return !runner_.isRunning(); };
    menu.addItem("run",
                 "Run",
                 [this, key]() { runFilter(key, false); },
                 not_running);
    menu.addItem("run-box",
                 "Run on the Box",
                 [this, key]() { runFilter(key, true); },
                 not_running);
    menu.addSeparator();
    menu.addItem("modify",
                 "Modify…",
                 [this, key]() { modifyFilter(key); });
    menu.addItem("export",
                 "Export…",
                 [this, key]() { exportFilter(key); });
    menu.addSeparator();
    menu.addItem("delete",
                 "Delete",
                 [this, key]() { deleteFilter(key); });
}

void FiltersPanelView::onVisibilityToggled(const QString&, bool) {
    // The filter tree has no visibility column.
}

void FiltersPanelView::setActionPipeline(bonsaiviewer::services::ActionPipeline* action_pipeline) {
    action_pipeline_ = action_pipeline;
}

void FiltersPanelView::onNodeDoubleClicked(const QString& key) {
    if (!key.isEmpty() && !store_.isFolder(key) && !runner_.isRunning()) {
        runFilter(key, false);
    }
}

void FiltersPanelView::onNodeMoved(const QString& key, const QString& new_parent) {
    // Only folders accept drops (TreeNode::accepts_children), so a non
    // folder parent can only come from a stale tree: ignore it.
    if (!new_parent.isEmpty() && !store_.isFolder(new_parent)) return;
    store_.moveNode(key, new_parent);
}

void FiltersPanelView::addFolder(const QString& parent_id) {
    const QString name = QInputDialog::getText(panel_, "New Folder", "Folder name");
    if (!name.trimmed().isEmpty()) {
        store_.addFolder(name.trimmed(), parent_id);
    }
}

void FiltersPanelView::addFilter(const QString& folder_id) {
    FilterDocument document;
    document.name = "New filter";
    openCreationDialog(document, {}, folder_id);
}

void FiltersPanelView::modifyFilter(const QString& filter_id) {
    const auto document = store_.filter(filter_id);
    if (!document) return;
    // Parse every selection layer back into facet fields, then open the
    // window. AND layers carry no fields to parse. The state is shared by
    // the parse callbacks; the first failure aborts and the others become
    // no-ops.
    struct ParseState {
        FilterDocument document;
        std::vector<FilterCreationDialog::EditLayer> layers;
        size_t remaining = 0;
        bool failed = false;
    };
    auto state = std::make_shared<ParseState>();
    state->document = *document;
    state->layers.resize(document->layers.size());
    for (size_t i = 0; i < document->layers.size(); ++i) {
        state->layers[i].is_and = document->layers[i].is_and;
        state->layers[i].actions = document->layers[i].actions;
        if (!document->layers[i].is_and && !document->layers[i].ids_xml.isEmpty()) {
            ++state->remaining;
        }
    }
    if (state->remaining == 0) {
        openCreationDialog(state->document, state->layers, document->folder_id);
        return;
    }
    for (size_t i = 0; i < document->layers.size(); ++i) {
        if (document->layers[i].is_and || document->layers[i].ids_xml.isEmpty()) continue;
        ids_bridge_.parse(document->layers[i].ids_xml,
                          [this, state, i, filter_id](bool ok,
                                                     const bonsaiviewer::services::IdsBridge::SelectionFields& fields,
                                                     const QString& error) {
            if (state->failed) return;
            if (!ok) {
                state->failed = true;
                services::popup::information(panel_, error.isEmpty()
                    ? "the filter's specification could not be read" : error);
                return;
            }
            state->layers[i].fields = fields;
            if (--state->remaining == 0) {
                const auto current = store_.filter(filter_id);
                if (!current) return; // deleted while parsing
                openCreationDialog(state->document, state->layers, current->folder_id);
            }
        });
    }
}

void FiltersPanelView::openCreationDialog(const FilterDocument& document,
                                          const std::vector<FilterCreationDialog::EditLayer>& layers,
                                          const QString& folder_id) {
    auto* dialog = new FilterCreationDialog(
        document, layers, ids_bridge_,
        [this]() { return bonsaiviewer::services::FilterRunner::activeIfcPath(session_state_); },
        panel_);
    // Preview "Isolate in the view": EXPRESS ids of the active model ->
    // object ids -> B08 isolate, like the filter's Isolate action.
    connect(dialog, &FilterCreationDialog::isolateRequested, this,
            [this](const std::vector<int>& express_ids) {
        if (action_pipeline_ == nullptr) return;
        uint32_t session_model_id = 0;
        bonsaiviewer::services::FilterRunner::activeIfcPath(session_state_, &session_model_id);
        if (session_model_id == 0 || session_state_.elementRegistry() == nullptr) return;
        QHash<int, uint32_t> object_id_by_ifc_id;
        for (const auto& info : session_state_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
            object_id_by_ifc_id.insert(info.ifc_id, info.object_id);
        }
        std::vector<uint32_t> object_ids;
        object_ids.reserve(express_ids.size());
        for (int id : express_ids) {
            const auto found = object_id_by_ifc_id.find(id);
            if (found != object_id_by_ifc_id.end()) object_ids.push_back(found.value());
        }
        action_pipeline_->run(object_ids, bonsaiviewer::services::ActionPipeline::Action::Isolate);
        session_state_.setStatusMessage("Filters", QString("%1 element(s) isolated by the preview")
                                                       .arg(object_ids.size()));
    });
    connect(dialog, &FilterCreationDialog::documentReady, this,
            [this, folder_id, is_new = document.id.isEmpty()](const FilterDocument& ready) {
        if (is_new) {
            store_.addFilter(ready, folder_id);
        } else {
            store_.updateFilter(ready);
        }
    });
    connect(dialog, &FilterCreationDialog::finished, dialog, &QObject::deleteLater);
    dialog->show();
}

void FiltersPanelView::runFilter(const QString& filter_id, bool on_box) {
    const auto document = store_.filter(filter_id);
    if (!document) return;
    panel_->setRunEnabled(false);
    session_state_.setStatusMessage("Filters", QString("running \"%1\"…").arg(document->name));
    runner_.run(*document, on_box ? bonsaiviewer::services::FilterRunner::Scope::Box
                                  : bonsaiviewer::services::FilterRunner::Scope::WholeModel);
}

void FiltersPanelView::deleteFilter(const QString& filter_id) {
    const auto document = store_.filter(filter_id);
    if (!document) return;
    if (services::popup::confirmation(
            panel_, QString("Delete the filter \"%1\"?").arg(document->name))) {
        store_.deleteFilter(filter_id);
    }
}

void FiltersPanelView::renameFolder(const QString& folder_id) {
    const auto entry = store_.folder(folder_id);
    if (!entry) return;
    const QString name = QInputDialog::getText(panel_, "Rename Folder", "Folder name",
                                               QLineEdit::Normal, entry->name);
    if (!name.trimmed().isEmpty()) {
        store_.renameFolder(folder_id, name.trimmed());
    }
}

void FiltersPanelView::deleteFolder(const QString& folder_id) {
    const auto entry = store_.folder(folder_id);
    if (!entry) return;
    const QString message = store_.folderIsEmpty(folder_id)
        ? QString("Delete the folder \"%1\"?").arg(entry->name)
        : QString("Delete the folder \"%1\" and everything inside it?").arg(entry->name);
    if (services::popup::confirmation(panel_, message)) {
        store_.deleteFolder(folder_id);
    }
}

void FiltersPanelView::exportFilter(const QString& filter_id) {
    const auto document = store_.filter(filter_id);
    if (!document) return;
    const QString path = QFileDialog::getSaveFileName(
        panel_, "Export Filter", document->name + ".json", "JSON (*.json)");
    if (!path.isEmpty() && !store_.exportFilter(filter_id, path)) {
        services::popup::information(panel_, "The filter could not be exported");
    }
}

void FiltersPanelView::importFilter(const QString& folder_id) {
    const QString path = QFileDialog::getOpenFileName(
        panel_, "Import Filters", {},
        "JSON (*.json);;All files (*)");
    if (path.isEmpty()) return;
    QString error;
    const int count = store_.importFile(path, folder_id, error);
    if (count < 0) {
        services::popup::information(panel_, error.isEmpty()
            ? "The filters could not be imported" : QString("The filters could not be imported: %1").arg(error));
        return;
    }
    session_state_.setStatusMessage("Filters", QString("%1 filter(s) imported").arg(count));
}

void FiltersPanelView::exportLibrary() {
    const QString path = QFileDialog::getSaveFileName(
        panel_, "Export All Filters", "filters.json", "JSON (*.json)");
    if (!path.isEmpty() && !store_.exportLibrary(path)) {
        services::popup::information(panel_, "The filters could not be exported");
    }
}

} // namespace bonsaiviewer::modules::filters
