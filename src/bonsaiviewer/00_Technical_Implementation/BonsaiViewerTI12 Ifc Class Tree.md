<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI12 - Ifc Class Tree

Spec source: `../00_Specification/BonsaiViewer12 Ifc Class Tree.md`

## What exists today

`modules/spatial_hierarchy` and the Models panel are tree panels, but there
is no reusable tree framework yet; each panel hand-rolls its model. This
fiche introduces the class tree as the second consumer of B12 (after the
spatial hierarchy refactor or together with it).

## Building blocks used

- B12 TreePanelFramework — tree with class nodes and instance leaves.
- B05 IfcDataAccess — IfcClass of each element (BasicElementInfo already
  carries it), merged across all open models.
- B01 SelectionService — click on instance selects; click on class selects
  all instances of the class.
- B02 + B08 — double-click on an instance isolates it.

## New code

- `modules/class_tree/`:
  - `ClassTreeDataSource` implementing B12's data source interface: builds
    `IfcClass → [instances]` from `ElementRegistry::basicElementInfoForModel`
    for every open model; only classes with instances appear; refresh hooks:
    `modelsChanged`, `modelGeometryReady`, `notifyFederationChanged`
    (spec: refreshed at each model load).
  - Panel factory registered in B13; toggle appears in the Window menu (TI01)
    and the Panels ribbon.

## Key flows

Model added → registry elements ready → data source rebuilds → tree view
refreshed. Click class node → B01.select(instances, source=tree).

## Technical questions

- Sorting of class nodes (alphabetical vs instance count) — pick one and
  note it in the data source.
- Large classes (tens of thousands of instances): lazy population of class
  children in B12 (expand on demand) rather than building all upfront.
