# BonsaiViewer08 - Orbit and Zoom

In Settings, under the Navigation menu, there are two other selection lists:
- Zoom Preset
- Orbit Preset

## Specification

### Zoom Preset
#### Zoom in Center
It's the current way to do. When you use the zoom function, it zoom in the center of the model.

#### Zoom in Cursor
The zoom function can zoom toward the mouse position. It needs to detect the first position under the mouse and zoom toward that point.
If there is nothing under the mouse, it falls back on the current behaviour (zoom in the center of the model).

#### Zoom on Selection
If one object is selected, the zoom needs to zoom toward the center of that object.
With several selected objects, the zoom goes toward the center of the bounding box of the selection.
With no selection, it falls back on the current behaviour (zoom in the center of the model).

### Orbit Preset
#### Orbit the Center of View
It's the current way to orbit. You can move the orbit position when you move the view.
The orbit position is always in the center of the view.

#### Orbit in the Cursor
The orbit center is located under the mouse position. It needs to calculate the first position under the mouse to determine the first object position and the position of the orbit.
The orbit center is computed at the start of each orbit drag, then stays fixed during the drag.
If there is nothing under the mouse, it falls back on the current behaviour (orbit the center of view).
