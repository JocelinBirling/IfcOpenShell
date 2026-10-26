<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI08 - Orbit and Zoom

Spec (human summary): `../00_Specification/BonsaiViewer08 Orbit and Zoom.md`

## Settings

- `src/bonsaiviewer/ViewerSettings.h/.cpp`
  - Enums `ZoomPreset`, `OrbitPreset`, with getters and setters.
  - QSettings keys `interface/navigation/zoom_preset` and `interface/navigation/orbit_preset`, default 0.
  - Signal `navigationPresetsChanged`.
- `src/bonsaiviewer/modules/settings/Dialog.h/.cpp`: combos `zoom_preset_combo_` and `orbit_preset_combo_` ("Zoom" / "Orbit") in the existing Navigation section, read on sync and saved in `onAccepted`.
- `src/bonsaiviewer/modules/viewport/View.cpp`: lambda `applyNavigationPresets`, called at construction and on `navigationPresetsChanged`. Maps the ViewerSettings enums to the ViewportWindow enums.

## Viewport

`src/ifcviewer/ViewportWindow.h/.cpp`

- Enums: `ViewportWindow::ZoomPreset { CenterView, Cursor, Selection }`, `OrbitPreset { CenterView, Cursor }`, with setters (`zoom_preset_`, `orbit_preset_`).
- `pickVisibleSurfaceAt(...)`: ray cast from the mouse position. Physical pixels = logical position x devicePixelRatio. Skips hidden objects (`ViewportCore::isObjectHidden`).
- `selectionCenter()`: centre of the bounding box of the selection, from `computeObjectAabb` (loaded instances, hidden included). Returns nothing if no valid bounds.

### Zoom (`wheelEvent`)

- CenterView: `dollyBy`.
- Cursor: `pickVisibleSurfaceAt`, then `dollyToward(hit)`. No hit: `dollyBy`.
- Selection: `selectionCenter()`, then `dollyBy`. No valid selection: `dollyBy`.
- `dollyBy` multiplies the distance by 0.9 per notch, minimum 0.01 m, no maximum. The Cursor zoom is exact only when the hit lies on the view axis.

### Orbit: Cursor (fixed pivot)

Angle conventions (`ViewportCore::orbitBy` and `orbitAroundPivot`): 0.4 deg per pixel, yaw -= dx, pitch += dy, pitch clamped to +/-89.9 deg.

- `orbitBasis(yaw, pitch, right, up, fwd)`: `fwd = -dir`, `right = fwd x UnitZ`, `up = right x fwd`, normalised.
- Press (Orbit branch): `orbit_pivot_active_ = false`. If `orbit_preset_ == Cursor`, `pickVisibleSurfaceAt(press position)`, then `startPivotOrbit(pivot)`.
- `startPivotOrbit(pivot)`:
  - `eye = target - camera_distance_ * fwd`;
  - `v = pivot - eye`;
  - `orbit_pivot_camera_ = (v.right, v.up, v.fwd)`;
  - sets `orbit_pivot_`, `orbit_pivot_active_ = true`;
  - draws the screen ball (below).
- Move: if `orbit_pivot_active_`, `orbitAroundPivot(dx, dy)`, else `core_.orbitBy`.
- `orbitAroundPivot(dx, dy)`:
  - updates yaw and pitch;
  - recomputes the basis;
  - `eye = orbit_pivot_ - (c.x*right + c.y*up + c.z*fwd)`;
  - `target = eye + camera_distance_ * fwd`;
  - writes `camera_target_`, requests a frame.
- Release: `orbit_pivot_active_ = false`, clears the ball.
- `focusOutEvent`: resets the drag state, `orbit_pivot_active_`, the overlay points, and the box-select flags, then calls `QWindow::focusOutEvent`.
- No hit on press: orbit around the view centre (`core_.orbitBy`).

### Screen ball

`setOverlayPoints({pivot}, fill, pixel_size, stroke, stroke_extra)`. Current values, untuned: pixel size 12, fill (1.0, 0.62, 0.10, 0.95), stroke (0.0, 0.0, 0.0, 0.85), stroke extra 1.5. Cleared on release and focus loss.

## Checks to do on screen

- The pivot stays under the cursor during the drag, with no jump at the click.
- Horizontal and vertical turn directions are correct (basis `right = fwd x Z` matches the renderer's view convention).
- Ball size and colour are readable.
