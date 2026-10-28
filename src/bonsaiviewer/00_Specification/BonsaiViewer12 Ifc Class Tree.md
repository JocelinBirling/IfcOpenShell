# BonsaiViewer12 - IFC Hierarchy

## Introduction

A class tree complements the existing trees. It lists the IFC classes present in the project and groups their instances by class. It shows all open models in one unified tree.

## Specification

### IFC Hierarchy panel

There is **one single tree** for all open models.

The tree shows only IFC classes that have instances. Instances from all open models are grouped together by IFC entity name, regardless of the model's IFC schema. For example, `IfcWall` instances from different models appear in the same class group.

- IfcWall (3)
  - Wall1
  - Wall2
  - Wall3
- IfcSlab (2)
  - Slab1
  - Slab2

Each class shows its number of instances. Class groups and instance names are sorted alphabetically. Instance names are displayed as provided by the model; an instance without a name shows its GlobalId. Duplicate names do not need an additional model label.

**Depth of the tree:** the nesting follows the class hierarchy of each schema (IFC2X3 and IFC4 do not have the same hierarchy). The exact nesting rule when IFC2X3 and IFC4 models are open at the same time is still to be defined (see Open Subject).

When no open model contains usable instances, show an explicit empty state without blocking the viewer.

Placement: the panel is docked on the left, tabbed with the Spatial Hierarchy, and is visible at start-up. It is also listed in the Panels ribbon and in the Window menu.

### Refresh and updates

Refresh the tree when a model is loaded or closed, and when changes to a model affect its instances. When a model changes, update only the affected classes and instances rather than rebuilding the entire tree.

### Interactions

The standard viewer conventions apply:

- **Click on an instance**: selects the element.
- **Double-click on an instance**: isolates the element.
- **Click on a class**: selects all instances of that class across all open models.
- **Tree expansion control**: expands or collapses a class group.
- **Right click**: menu **Isolate**, on a class (all its instances) or on an instance.

## Open Subject

- Nesting rule per schema: how to build the hierarchy when IFC2X3 and IFC4 models are open together (one tree, depth depending on each schema).
- Large classes (tens of thousands of instances): the tree builds the class children at each refresh; a lazy build on expand is still to do.
