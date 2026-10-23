<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI13 - Unique Tree

Spec source: `../00_Specification/BonsaiViewer13 Unique Tree.md`

## What exists today

`modules/spatial_hierarchy` shows the classic per-model spatial tree. The
clash panel exists for clash detection. No cross-model hierarchy
reconstruction exists.

## Building blocks used

- B12 TreePanelFramework — the "Unified Spatial Hierarchy" panel, same
  conventions as the classic panel.
- B05 IfcDataAccess — spaces, storeys, element positions per model.

## New code

`modules/unified_tree/` with two stages:

- **Assignment engine** (pure, testable, no UI):
  - *Rooms:* for each non-spatial element, find the spaces that contain it.
    Fast path: oriented bounding box (OBB) containment test against the
    spaces of the reference model; if exactly one candidate passes, stop. If
    several, refine (element OBB vs space OBB overlap ratio). Elements can
    attach to several rooms.
  - *Levels:* the reference model's storeys define elevation intervals;
    each element is attached by the elevation of its position. Slab rule: a
    slab just below a level is attached to the level above it (elevation +
    tolerance band).
  - Output: `element_id → {space_ids, storey_id}` map per federation.
  - Incremental: on model add, re-run; if settings unchanged, only index the
    new models (map merge).
- **Settings:** two reference-model selectors (levels, rooms) in
  ViewerSettings; default = the open model with the most IfcSpaces.
- **Panel:** a second tree panel (B12) docked beside the classic spatial
  hierarchy, with a model dropdown (Settings) and a Run button; the tree is
  a federation-wide "fictive Spatial Hierarchy" built from the assignment map.

## Key flows

Add model → engine indexes the new model only → unified tree refreshed.
Settings changed → full re-run on Run click.

## Technical questions

- OBB source: if the loaded geometry does not carry OBBs, compute them from
  element bounding boxes at index time (cost on large models; make the run
  cancellable with the SessionState progress bar).
- Element position: use the element's bounding box center, or its
  representation origin? Needs a decision for slabs hanging below their
  level.
