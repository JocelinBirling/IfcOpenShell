<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI02 - Display Function

Spec source: `../00_Specification/BonsaiViewer02 Display Function.md`

## What exists today

`modules/viewport/Commands.cpp` implements visibility commands (hide,
isolate, show all) on the current single-plane section mode; the viewport
panel has Length / Area / Volume tools and one section plane. The existing
visibility commands become wrappers of B02.

## Building blocks used

- B02 ActiveView — Show/Hide Selection, Show All, Hide All, invert.
- B03 CategoryOverrideFilter — quick category menu (Walls, Slabs, Roofs,
  Ceilings, Spaces, Openings); re-applied after Show All.
- B04 AppearanceService — Paint, Remove Color, Transparency.
- B08 ActionPipeline — "Select everything in view" (B01 from B02) and the
  colour actions.
- B09 PickingService — surface normal for section creation and perpendicular
  view.
- B10 CameraService — Fit View (200 m heuristic), Perpendicular View.
- B11 SectionPlaneManager — multi-plane cuts, gizmos, hide, suspend, remove.
- B17 PopupService — "The objects are far away", "The active view is empty".

## New code

- `services/SectionPlaneManager` (B11): plane list (origin, normal, visible,
  suspended), viewport gizmo entities with drag-to-translate and right-click
  "Delete plane" (B15), knife cursor during creation, set-square cursor during
  perpendicular view, auto-deactivation after one creation.
- `services/AppearanceService` (B04): `setColour(ids, colour)`,
  `removeColour(ids)` (keeps transparency), `setOpacity(ids, pct)`,
  priority layers, reset on model reload (hook `modelGeometryReady`).
  Overrides mark themselves above clash-run colouring until the next clash
  run.
- `services/CameraService` (B10): `fitView()` computing bounds of B02's
  visible set, then the 200 m heuristic (raycast / bounds test against the
  view plane; popup if empty result), `framePerpendicular(normal)`.

## Key flows

Add section: click button → knife cursor → next LMB click → B09 returns
surface normal → B11 adds plane (perpendicular to normal, facing it) →
cursor restored. All planes of B11 clip simultaneously in the render
pipeline; Suspend hides gizmos and disables clipping; Hide only hides the
gizmos.

## Technical questions

- Render pipeline support for multiple simultaneous clip planes: verify the
  wgpu viewer's clipping API capacity before promising N planes.
- Transparency presets (100/75/50/25) live in a small dialog; reuse
  `components/Dialog`.
