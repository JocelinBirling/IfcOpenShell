# BonsaiViewer12 - Ifc Class Tree

## Introduction

A classification tree completes the existing trees. It displays the IFC classes in the project and groups their instances by class. The tree contains all open models in one unified view.

## Specification

### Ifc Class Tree panel

The tree shows only IFC classes that have instances. Instances from all open models are grouped together by IFC entity name, regardless of the IFC schema version. For example, `IfcWall` instances from different models appear in the same class group.

- IfcWall (3)
  - Wall1
  - Wall2
  - Wall3
- IfcSlab (2)
  - Slab1
  - Slab2

Each class shows its number of instances. Sort class groups and instance names alphabetically. Display instance names as provided by the model; an instance without a name shows its GlobalId. Duplicate names do not need additional model labels.

There is one tree for all open models. When no open model contains usable instances, show an explicit empty state without blocking the viewer.

Placement: the panel is docked on the left, tabbed with the Spatial Hierarchy, and is visible at start-up. It is also listed in the Panels ribbon and in the Window menu.

### Refresh and updates

Refresh the tree when a model is loaded or closed, and when changes to a model affect its instances. When a model changes, update only the affected classes and instances rather than rebuilding the entire tree.

### Interactions

The standard viewer conventions apply:

- **Click on an instance** — selects the element.
- **Double-click on an instance** — isolates the element.
- **Click on a class** — selects all instances of that class across all open models.
- **Use the tree expansion control** — expands or collapses a class group.
- **Right click** — menu **Isolate**, on a class (all its instances) or on an instance.

## Open Subject

- Large classes (tens of thousands of instances): the tree builds the class children at each refresh; a lazy build on expand is still to do.
