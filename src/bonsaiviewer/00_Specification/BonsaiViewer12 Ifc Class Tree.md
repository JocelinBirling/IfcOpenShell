# BonsaiViewer12 - Ifc Class Tree

## Introduction

A classification tree completes the existing trees. It displays the IFC classes in the project and groups their instances by class. The tree contains all open models in one unified view.

## Specification

### Ifc Class Tree panel

The tree shows only IFC classes that have instances. Instances from all open models are grouped together by IFC entity name, regardless of the IFC schema version. For example, `IfcWall` instances from different models appear in the same class group.

- IfcWall
  - Wall1
  - Wall2
  - Wall3
- IfcSlab
  - Slab1
  - Slab2

Sort class groups and instance names alphabetically. Display instance names as provided by the model; duplicate names do not need additional model labels.

There is one tree for all open models. When no open model contains usable instances, show an explicit empty state without blocking the viewer.

### Refresh and updates

Refresh the tree when a model is loaded or closed, and when changes to a model affect its instances. When a model changes, update only the affected classes and instances rather than rebuilding the entire tree.

### Interactions

The standard viewer conventions apply:

- **Click on an instance** — selects the element.
- **Double-click on an instance** — isolates the element.
- **Click on a class** — selects all instances of that class across all open models.
- **Use the tree expansion control** — expands or collapses a class group.

## Open Subject
