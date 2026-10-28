# BonsaiViewer32 - Tree Explorer

Status: draft, to validate.

## Introduction

The viewer has several trees: Spatial Hierarchy, IFC Class Tree (BonsaiViewer12), Unified Tree (BonsaiViewer13), and the System Hierarchy to come (BonsaiViewer28). Each one is a separate panel with its own code and its own interactions. The Tree Explorer gathers every tree in **one panel**: a selector chooses the tree, and every tree shares the same interactions. A new tree is a new data source, not a new panel.


It's maybe usefull to keep Spatial Tree un USpatial and Class in a specific panel. It's usefull for beginers. The System, Group and Classifications are for advanced users.

The Tree Explorer can be usefull to gain space in the UI, for advance user. From the Tree Explorer, we should be able to display any tree.



## Specification

### Panel

One panel, "Trees", docked on the left. At the top:

- **Tree selector**: a dropdown listing the trees below. The last choice is remembered.
- **Search field**: filters the nodes by name; the parents of a match stay shown.
- **Scope**: "All models" or "Active model", for the trees that support both.
- For an interpreted tree: the banner "Deduced data: not read from the models" (BonsaiViewer13).

Several Trees panels can be opened (Window menu, "New Tree Panel"), each with its own tree, to compare two trees side by side.

### Trees

| Tree | Hierarchy | Source | Interpreted |
|---|---|---|---|
| Models | Federation groups > Models | .ifcfed | No |
| Spatial | Model > Site > Building > Storey > Space > Elements | IFC relations | No |
| Classes | IFC class > Instances (BonsaiViewer12) | .ifcview | No |
| Types | IFC class of the type > Type > Occurrences | IFC relations | No |
| Systems | System > Sub-systems > Members (IfcSystem, IfcDistributionSystem...) | IFC relations | No |
| Zones | Zone > Spaces | IFC relations | No |
| Groups | Group > Members (IfcGroup, IfcAsset, IfcInventory) | IFC relations | No |
| Materials | Material > Elements | IFC relations | No |
| Unified | Site > Building > Unified level > Room > Elements (BonsaiViewer13) | Computation | Yes |

Trees read from the IFC relations are available once the IFC data source of a model is open; until then the tree shows the models that are still loading. Nothing is saved: the trees are rebuilt from the models (BonsaiViewer14, Storage).

### Shared interactions

The same for every tree:

- **Click** on an element: selects it. On a folder (class, storey, system...): selects all the elements below it.
- **Double-click**: isolates the node and everything below it.
- **Eye column**: shows or hides the node and everything below it.
- **Right click**: Isolate, Select, Hide, Show; and **Show properties**, which opens the node's entity in the Properties panel (BonsaiViewer14), with or without geometry.
- **Counts**: each folder shows the number of elements below it. An element listed in several places (several systems, several rooms) is counted once per folder, and carries the mark "×N" (BonsaiViewer13).
- **Large folders**: children are built when the folder is expanded.

### Existing panels

The Spatial Hierarchy and IFC Class Tree panels become the trees "Spatial" and "Classes" of the Tree Explorer, with the same content and behaviour. To decide: remove the two old panels, or keep them as shortcuts that open a Trees panel on that tree.

### Implementation note

The panel is built on the existing TreePanelFramework (TreeDataSource): one data source per tree. The class tree already uses it; the Spatial Hierarchy is migrated to it.

## Open Subject

- Remove or keep the old Spatial Hierarchy and IFC Class Tree panels.
- Custom tree ("Group by"): the user stacks levels (model, unified level, class, system, Pset.Property...). Proposed, not retained for now.
- Materials tree: material sets (layers, profiles, constituents) shown as one node or detailed.
- Manual editing of the Unified tree (BonsaiViewer13, later version).



