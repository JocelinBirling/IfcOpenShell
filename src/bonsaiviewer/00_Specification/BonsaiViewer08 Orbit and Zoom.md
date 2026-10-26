# BonsaiViewer08 - Orbit and Zoom

In Settings, under the Navigation menu, there are two selection lists:
- Zoom Preset
- Orbit Preset

## Specification

### Zoom Preset

#### Zoom in Center
This is the current behavior. Zoom toward the current camera target.

#### Zoom in Cursor
Zoom toward the first surface hit by a ray cast from the mouse position. If no surface is hit, fall back to the current camera target.

#### Zoom on Selection
With one selected object, zoom toward the center of that object's bounding box. With multiple selected objects, zoom toward the center of the bounding box enclosing all selected objects, including hidden objects.

Ignore selected objects that are no longer loaded or do not have a valid bounding box. If no valid selected objects remain, zoom toward the current camera target.

### Orbit Preset

#### Orbit the Center of View
This is the current behavior. Orbit around the current center of view; the orbit position moves with the view.

#### Orbit in the Cursor
At the start of each orbit drag, cast a ray from the mouse position and use the first visible surface hit as the orbit center. Keep that center fixed for the duration of the drag, even if the camera or view changes.

If the ray hits nothing, fall back to orbiting around the center of view.

Continue the drag if the cursor leaves the viewer window, until the mouse button is released. If the viewer loses focus before receiving the release event, cancel the drag cleanly and release any captured input state.

### Camera constraints
All zoom presets must respect the camera's existing minimum and maximum zoom limits.
