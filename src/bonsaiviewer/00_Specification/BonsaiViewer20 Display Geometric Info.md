# BonsaiViewer20 - Display Geometric Info

## Introduction
A right click on an object in the 3D view opens the object context menu. It holds a "Geometric Info" sub-menu, with the display functions described here and a few more display functions.

The display functions are toggles: they add or remove the graphic on the object, and the setting stays until it is explicitly removed. Each graphic is tied to its object.

The display is graphic only: no numeric readout of the values.

## Specification

Those function can be applied to one object or with a selection of several objects.

### Context Menu
- Right click on an object opens the object context menu.
- The menu contains a "Geometric Info" sub-menu.
- The sub-menu holds the display functions below, plus other display functions.

### Show/Remove the Insertion Point
- The toggle adds the insertion point marker of the object in the view.
- The marker is tied to the object in question and follows it.
- The marker stays displayed until the toggle removes it.

### Show/Remove the Bounding Box
- The toggle adds the bounding box of the element in the view.
- The box shown is the bounding box present in the representation of the object in the IFC, when it exists.

### Show/Remove Alternative Geometry
We could switch from representation here. 
TO BE DETERMINED


## Open Subject
The other display functions of the "Geometric Info" sub-menu need to be listed.
