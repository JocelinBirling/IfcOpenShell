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

#ifndef IFCINTERFACE_MODULES_CLASSTREE_CLASSTREEDATASOURCE_H
#define IFCINTERFACE_MODULES_CLASSTREE_CLASSTREEDATASOURCE_H

#include "../../services/TreePanelFramework.h"

#include <QHash>
#include <QMap>
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

namespace bonsaiviewer::modules::class_tree {

// TI12 — the Ifc Class Tree data source (B12): the IFC classes that have
// instances, each with its instances, over all the open models. Instances
// are grouped by IFC class name, whatever the schema. The index is kept per
// model: a model event rebuilds that model only.
class ClassTreeDataSource : public bonsaiviewer::services::tree::TreeDataSource {
    Q_OBJECT

public:
    ClassTreeDataSource(bonsaiviewer::SessionState& session,
                        bonsaiviewer::services::SelectionService& selection,
                        bonsaiviewer::services::ActionPipeline& actions,
                        QObject* parent = nullptr);

    // tree::TreeDataSource
    std::vector<bonsaiviewer::services::tree::TreeNode> topLevelNodes() override;
    std::vector<bonsaiviewer::services::tree::TreeNode> childrenOf(const QString& key) override;
    void populateContextMenu(const QString& key,
                             bonsaiviewer::services::ContextMenuBuilder& menu) override;
    // Double-click on an instance isolates it.
    void onNodeDoubleClicked(const QString& key) override;
    void onVisibilityToggled(const QString& key, bool visible) override;

    // The tree's selection changed: the viewport selection becomes the
    // elements of every selected node (a class node stands for its instances).
    void onSelectionChanged(const QStringList& keys);

private:
    struct Instance {
        std::uint32_t object_id = 0;
        QString label;
    };
    // IFC class name -> its instances, for one model.
    using ClassIndex = QMap<QString, std::vector<Instance>>;

    void rebuildModel(std::uint32_t session_model_id);
    void rebuildAll();
    // The object ids of every instance of a class, over all the models.
    std::vector<std::uint32_t> idsOfClass(const QString& ifc_class) const;
    // The object ids of a set of node keys (a class node: all its instances).
    std::vector<std::uint32_t> idsOfKeys(const QStringList& keys) const;

    bonsaiviewer::SessionState& session_;
    bonsaiviewer::services::SelectionService& selection_;
    bonsaiviewer::services::ActionPipeline& actions_;
    QHash<std::uint32_t, ClassIndex> models_;
    // The tree's selection (Ctrl / Shift clicks), kept for the menu.
    QStringList selected_keys_;
};

} // namespace bonsaiviewer::modules::class_tree

#endif // IFCINTERFACE_MODULES_CLASSTREE_CLASSTREEDATASOURCE_H
