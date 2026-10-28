# BonsaiViewer11 - Distance Function

## Introduction

The Distance tool measures distances between geometric entities: points (vertices), segments (edges) and faces. When Distance is activated, the cursor changes to a tape-measure shape.

## Specification

### Hover button spec
When you hover your mouse on the Distance Function, it needs to display a small info on how to use the distance tools.


### Hover feedback
Depending on the cursor position, the tool highlights the entity that can be picked; the screen shows the type of object being hovered.

- **Point** — when hovering a vertex; the point is highlighted in black.
- **Segment** — when hovering an edge; the segment is highlighted in black.
- **Face** — when hovering a face; a triangle is drawn around the location.


A right click will active the actual function of the software (And produce Z Extend and Y Extend)
A left click will select the point, segment or face.

### Measuring
A left click saves the point; a line is then drawn between the mouse cursor and this point. The second point of the measurement must next be determined. The measurement method depends on the types of the two selected entities:

- **Point–point** — absolute distance between the two points.
- **Point–segment** — distance along the normal to the segment connecting the point.
- **Point–face** — normal to the surface meeting the point; this is the shortest distance.
- **Segment–segment** — distance between the two segments.
- **Segment–face** — to be defined (see Open Subject).
- **Face–face** — if the two faces are coplanar within 1 degree: distance between the two planes; if not coplanar, the case is treated as a face–point selection.

Measurements persist during the session and can be deleted one by one.

### Context menu
A left click on a measurement selects it. Once selected, a right click opens the following functions:
- Delete the dimension
- Increase the text size
- Decrease the text size

### Units
The precision and the unit display are configurable via a settings button: change the number of decimals, show or hide the unit.

## Open Subject
- Get the full function that are already in it. It may be enough but a little bit hard to use.
