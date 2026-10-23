# BonsaiViewer02 - Display Function

## Introduction

The goal here is to specify the basic functions of a viewer. Users must be able to paint elements, manage section planes, and show and hide elements. It is a set of very general functions, but ones that will be used often.

## Specification

### Section (section cuts)

Cut the model with one or several planes, control their visibility and effect, and move or delete them directly in the viewport. Planes are created by clicking a surface and can coexist; all cuts apply simultaneously.

#### Add Section
Clicking on "Add a section plane" changes the cursor into a small knife.
On the next left click, the normal of the surface just clicked is looked up.
A section plane is produced, perpendicular to the normal of the surface, facing the same way as the normal.
After the creation of a cut, the cursor returns to its initial selection functionality and its original shape.
#### Remove Sections
This function deletes all the section planes present in the view.
The cut may be hidden or not, it will be deleted.
#### Hide Planes
This button hides the section plane object. The cut is still there, but the section plane is no longer visible. When clicked, the button must change its look (highlighted outline or changed icon), so it is clear that the button is active.

It is a toggle button. On the 1st click, the section plane is hidden.
On the 2nd click, the plane objects are displayed again.
#### Suspend Sections
This button suspends the section planes. The section planes are hidden and the cut is stopped.

It is a toggle button. On the 1st click, the cuts are suspended.
On the 2nd click, the section planes are reactivated.
#### Move a plane
The section plane can be translated by clicking on it and keeping the button pressed. The plane then moves one way or the other.
#### Delete one plane
right-click a plane gizmo and choose « Delete plane ».

### Colour

Change the appearance of the selected elements, with colour and opacity as two independent settings. Both reset on model reload and override Clash Filter colouring until a new clash run.

- **Paint** — opens a colour dialog and colours every selected element on OK.
- **Remove Color** — restores each element's original baked colour, keeping its transparency.
- **Transparency** — opens a preset dialog (100 / 75 / 50 / 25 % opacity); 100 % restores the original opacity.

### Display

Control element visibility globally and per IFC category. A disabled category can **never** be shown while the deactivation is active, whatever the path (selection, isolations, search, Show All).

- **Show Selection** — shows the selected elements.
- **Hide Selection** — hides the selected elements.
- **Show All** — shows everything, then re-applies the category filters.
- **Hide All** — hides every element of the active view.
- **Select Everything in view** - select every displayed element of the actual view
- **Categories (menu)** — « Show / Hide » toggles for Walls, Slabs, Roofs, Ceilings, Spaces, Openings
	- It's useful to quickly filter the view for neophyte user. 

### Zoom

Frame the visible objects and aim the camera at a picked surface. Both commands work on the **visible** elements, so a fit after an isolation zooms in on the isolated objects.
#### Fit View
On the click, the camera zooms to include all the objects of the view.

After the positioning, the viewer must look for one object within 200 m of the view plane.
If it finds at least 1 object within 200 m, there is nothing to do and it can stop searching.
If it does not find an object within 200 m, it must show a pop-up: "The objects are far away."

If no object is present in the view, a pop-up opens and shows the message: "The active view is empty."

#### Perpendicular View
On the click, the cursor changes into a set square.
Then, on the first right click, the normal is retrieved.
The view plane must be perpendicular to the normal, in the direction opposite to the normal.
After the move of the view plane, the function deactivates and the cursor returns to its original shape.




# Open Subject
