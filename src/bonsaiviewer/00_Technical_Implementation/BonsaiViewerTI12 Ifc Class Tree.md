<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI12 - Ifc Class Tree

Spec source: `../00_Specification/BonsaiViewer12 Ifc Class Tree.md`

## What exists today

`modules/spatial_hierarchy` and the Models panel are tree panels that hand-roll
their model. The class tree is built on the B12 framework
(`services/TreePanelFramework`), the same way as the Tables tree (TI07).

## Building blocks used

- B12 TreePanelFramework — `TreePanelWidget` renders the tree, the data source
  gives the nodes and the menus.
- B05 / ElementRegistry — `basicElementInfoForModel` gives each element's IFC
  class (`type`), name and GlobalId.
- B01 SelectionService — click on an instance or a class (`SelectionSource::Tree`).
- B08 ActionPipeline — double-click and the menu "Isolate" (`Action::Isolate`).

## Code

- `modules/class_tree/ClassTreeDataSource` (TreeDataSource):
  - Index per model: `QHash<session model id, QMap<IFC class, instances>>`.
    `rebuildModel(id)` rebuilds one model; `rebuildAll()` rebuilds every open
    model. A model no longer open is removed from the index.
  - Keys: a class node is `class/<IfcClass>`, an instance node is its object id.
  - Labels: a class shows `IfcWall (42)`; an instance shows its name, or its
    GlobalId when the name is empty. Classes and instances are sorted
    alphabetically, case-insensitive.
  - Refresh: `modelGeometryReady` and `modelDataSourceReady` rebuild the model
    concerned; `projectOpened` and `projectReset` rebuild all; each then emits
    `nodesChanged`.
- `MainWindow::setupPanels`: the panel `class_tree_panel_` (a `components::Panel`
  around a `QTreeWidget`), docked left and tabbed with `spatial_panel_`, visible
  at start-up.
- `MainWindow::setupLoader`: creates the data source and the
  `TreePanelWidget`, then calls `refresh()` once.
- `MainWindow::buildPanelsRibbonPage`: toggle "Class Tree" in the DATA group.
- `MainWindow::setupMenuBar`: adopted by the PanelManager as "classtree" (Window menu).

## Key flows

- Model loaded → `modelGeometryReady` → the model's classes are rebuilt →
  `nodesChanged` → the tree is refreshed.
- Click on a class → `SelectionService::setSelection(all ids of the class, Tree)`.
- Double-click on an instance → `ActionPipeline::run({id}, Isolate)`.

## Known limits

- `TreePanelWidget::refresh` rebuilds the whole tree, and it builds the children
  of every class at each refresh. A lazy build on expand (spec: large classes)
  is still to do; it needs a change in the B12 framework.
- A model removed without `projectReset` is only dropped at the next rebuild of
  another model.

## Technical questions

- Sorting: alphabetical by class name (decided); instance count is shown in the
  label, not used for sorting.
