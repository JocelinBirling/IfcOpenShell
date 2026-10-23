<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI08 - Orbit and Zoom

Spec source: `../00_Specification/BonsaiViewer08 Orbit and Zoom.md`

## What exists today

Zoom orbits on the center of the model / center of the view (current preset).
Navigation presets (Blender / Rhino / Revit / Web) already exist in settings.

## Building blocks used

- B09 PickingService — position and object under the mouse.
- B10 CameraService — preset behaviour and fallbacks.
- Settings dialog (`modules/settings`) — two new dropdowns under Navigation.

## New code

- `services/CameraService` zoom presets:
  - *Zoom in Center* — current behaviour, unchanged.
  - *Zoom in Cursor* — at each wheel step, B09 raycast under the cursor; zoom
    toward the first hit position; fall back to center when nothing is hit.
  - *Zoom on Selection* — zoom toward the center of B01's selection (single
    object center; several objects = center of the selection bounding box);
    fall back to center when the selection is empty.
- `services/CameraService` orbit presets:
  - *Orbit the Center of View* — current behaviour.
  - *Orbit in the Cursor* — at drag start, B09 raycast fixes the orbit pivot
    on the first hit; the pivot stays fixed during the drag; fall back to
    view center on no hit.

Settings: two enums (ZoomPreset, OrbitPreset) persisted in ViewerSettings,
combo boxes generated in the settings dialog.

## Key flows

Wheel / drag events in the viewport delegate to B10 with the active preset;
B09 results are cached per event to avoid double raycasts.

## Technical questions

- Raycast cost during wheel zoom on heavy scenes: consider throttling or
  using the depth buffer value under the cursor instead of a geometry
  raycast (cheaper, same UX).
