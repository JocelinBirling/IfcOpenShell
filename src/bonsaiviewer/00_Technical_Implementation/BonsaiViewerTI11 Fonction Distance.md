<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI11 - Distance Function

Spec source: `../00_Specification/BonsaiViewer11 Fonction Distance.md`

## What exists today

`Measurement.cpp` has LengthMeasurement (point chain) and AreaMeasurement
with the onPick / clear / rebuildOverlay pattern, HUD readout and
volume labels. The Distance tool here is a new entity-based measurement on
the same pattern.

## Building blocks used

- B09 PickingService — entity-level results: vertex, edge, face; hover
  highlight (black point, black segment, triangle around the face location);
  screen type indicator.
- B15 ContextMenuBuilder — menu on a selected measurement.
- B17 — none.

## New code

- `modules/measure/DistanceMeasurement` (new class beside the existing ones):
  - Hover: B09 reports the hit type; the tool highlights the pickable
    entity and shows its type; tape-measure cursor while active.
  - First LMB saves entity A; a rubber line follows the cursor until the
    second pick.
  - Measurement table: pair of typed entities → value, using a small
    geometry helper header:
    - point-point: euclidean distance;
    - point-segment: perpendicular distance to the segment;
    - point-face: shortest distance to the plane surface (normal at the
      point);
    - segment-segment: segment to segment distance;
    - face-face: if planes coplanar within 1 degree, plane distance; else
      degrade to face-point case;
    - segment-face: left explicit pending the spec's open subject.
  - Measurements persist during the session; LMB on a measurement selects
    it; right-click opens the context menu (delete dimension, increase /
    decrease text size); deletion one by one.
- Settings: precision (decimals) and unit display toggle in ViewerSettings,
  applied to the HUD and label rendering.

## Key flows

Activate → hover feedback loop (B09) → pick A → pick B → measurement
computed and stored, rendered as overlay; the tool stays active for chained
measurements until Esc / tool switch.

## Technical questions

- Picking granularity (vertex / edge / face) must be exposed by the render
  engine's raycast: verify the ifcviewer API provides hit triangle / edge /
  vertex data, and at which pick radius (screen-space tolerance).
- Hover indicator "the screen shows the type of object being hovered":
  agree the widget (status bar field vs HUD).
