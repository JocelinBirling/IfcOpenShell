# BonsaiViewer11 - Distance Function

## Introduction

The Distance tool measures distances between geometric entities: points (vertices), segments (edges) and faces. When Distance is activated, the cursor changes to a tape-measure shape.

## Specification

### Hover button spec
When you hover your mouse on the Distance Function, it needs to display a small info on how to use the distance tools.

The hover text of the three measure buttons (implemented):
- **Distance** (L): left click two entities (point, segment or face): the distance between them. Right click a surface: laser measure (extents, height). Shift + click several points: angle (3 points) or polygon area (4+). Backspace removes the last point, Esc exits.
- **Area** (A): click a face: adds its flat patch to the total; click it again to remove it. Alt + click: one triangle only. Esc exits.
- **Volume** (V): click or drag a box over objects: the volume of the selection, per object and in total. Shift / Ctrl add / remove. Esc exits.


### Hover feedback
Depending on the cursor position, the tool highlights the entity that can be picked; the screen shows the type of object being hovered.

- **Point** — when hovering a vertex; the point is highlighted in black.
- **Segment** — when hovering an edge; the segment is highlighted in black.
- **Face** — when hovering a face; a triangle is drawn around the location.


- **Left click** selects the point, segment or face.
- **Right click** on a surface runs the laser measure of the current tool: the extents of the face and, on a floor or ceiling, the height.
- **Right click** on a measurement opens its context menu (see Context menu).
- **Shift + click** keeps the former polyline mode: 3 points give the angle at the middle point, 4 points or more the polygon area. Backspace removes the last point.
- Only the real edges of the geometry are proposed as segments, not the internal diagonals of the triangulated faces.

### Measuring
A left click saves the point; a line is then drawn between the mouse cursor and this point. The second point of the measurement must next be determined. The measurement method depends on the types of the two selected entities:

- **Point–point** — absolute distance between the two points.
- **Point–segment** — distance along the normal to the segment connecting the point.
- **Point–face** — normal to the surface meeting the point; this is the shortest distance.
- **Segment–segment** — distance between the two segments.
- **Segment–face** — shortest distance: if the segment is parallel to the face, the distance between them; otherwise the shortest distance between the segment and the face (0 when the segment crosses it).
- **Face–face** — if the two faces are coplanar within 1 degree: distance between the two planes; if not coplanar, the case is treated as a face–point selection.

Measurements persist during the session and can be deleted one by one.

### Context menu
A left click on a measurement selects it. Once selected, a right click opens the following functions:
- Delete the dimension
- Increase the text size
- Decrease the text size

### Units
The precision and the unit display are configurable in File > Settings, section **Measurements**: change the number of decimals, show or hide the unit. Saved between sessions.

## Open Subject
- Implementation in two steps: (1) hover feedback and two-entity measurement; (2) persisted measurements, context menu, units. Done so far: the hover text of the buttons.
- Hover, right click and Shift + click need changes in the shared 3D viewport (ifcviewer: ViewportWindow, OverlayRenderer for the label size).
