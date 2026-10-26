# BonsaiViewer08 - Orbit and Zoom

In Settings, Interface tab, Navigation section, two lists: **Zoom** and **Orbit**. Both are saved and applied immediately.

## Zoom
- **Center**: zoom toward the centre of the view (current behaviour).
- **Cursor**: zoom toward the visible surface under the mouse. Hidden objects are ignored. If nothing is hit, zoom toward the centre of the view.
- **Selection**: zoom toward the centre of the selection, hidden objects included. Objects no longer loaded are ignored. If no valid selection remains, zoom toward the centre of the view.

## Orbit
- **Center**: orbit around the centre of the view (current behaviour).
- **Cursor**: at the start of a drag, the visible surface under the mouse becomes the pivot. The pivot stays fixed on screen during the drag, and a small ball marks it. If nothing is hit, orbit around the centre of the view.

A drag continues outside the viewer until the button is released. If the viewer loses focus, the drag is cancelled.

Zoom keeps the existing minimum distance (0.01 m). There is no maximum.
