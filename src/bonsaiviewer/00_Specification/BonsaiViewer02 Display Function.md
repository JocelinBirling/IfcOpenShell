# BonsaiViewer02 - Display Function

## Introduction

The goal is to specify the basic functions of the viewer. Users must be able to paint elements, manage section planes, show and hide elements, and go back a few steps. These are general functions, used often.

## Specification

### Section (section cuts)

Cut the model with one or several planes, control their visibility and effect, and move or delete them directly in the viewport. Planes are created by clicking a surface and can coexist; all cuts apply simultaneously.

#### Add Section
Clicking the "Section" button changes the cursor into a small knife.
On the next left click, the normal of the surface just clicked is read.
A section plane is created, perpendicular to that normal and facing the same way.
After the cut is created, the tool deactivates and the cursor returns to its normal selection behaviour and shape.

#### Add Cut Box
On the selection of an object, the cut box creates 6 section cuts covering every side of the bounding OBB.

#### Modify (menu)
The "Modify" button groups the three section maintenance functions in one dropdown, so the section group stays compact:

- **Remove Section**: deletes all section planes in the view, whether hidden or not.
- **Hide Section**: a toggle that hides the section plane objects. The cut stays active, but the planes are not visible. The checked state of the menu item shows the active state: 1st activation hides the planes, 2nd activation shows them again.
- **Suspend Section**: a toggle that suspends the section planes. The planes are hidden and the cut is stopped. The checked state shows the active state: 1st activation suspends the cuts, 2nd activation reactivates the planes.

#### Move a plane
The section plane can be translated by clicking on it and holding the button. The plane then moves in either direction.

#### Delete one plane
Right-click a plane gizmo and choose "Delete plane".

### Colour

Change the appearance of the selected elements, with colour and opacity as two independent settings. Both reset on model reload and override Clash Filter colouring until a new clash run.

- **Paint**: opens a colour dialog and colours every selected element on OK.
- **Remove Colour**: restores each element's original baked colour, keeping its transparency.
- **Transparency**: opens a preset dialog (100 / 75 / 50 / 25 % opacity). 100 % restores the original opacity.

### Display

Control element visibility globally and per IFC category. A disabled category can **never** be shown while the deactivation is active, whatever the path (selection, isolations, search, Show All).

- **Show Selection**: shows the selected elements.
- **Hide Selection**: hides the selected elements.
- **Show All**: shows everything, then re-applies the category filters.
- **Invert View**: flips the visibility of every element. The category filters are re-applied, so a disabled category cannot come back through this path.
- **Select Everything in view**: selects every displayed element of the current view.
- **Restore the View**: resets the display state in one click: removes every colour and transparency override and shows every object and every model again.
- **Categories (menu)**: "Show / Hide" toggles for Walls, Slabs, Roofs, Ceilings, Spaces, Openings. Useful to quickly filter the view for new users.

### Undo View (CTRL+Z)

A simple step-back on the display and colour state.

- **What is stored**: for every object, its visibility, colour and transparency override.
- **When a state is stored**: before each action that changes visibility or colour (Show, Hide, Show All, Invert View, Restore the View, Paint, Remove Colour, Transparency, isolations from the Properties panel, Table or Box). The state before the action is kept.
- **Depth**: 10 states at most. The oldest state is dropped when a new one is added.
- **CTRL+Z**: restores the previous state. With no history, nothing happens.
- **Not stored**: section planes, camera, selection, filter runs, table runs. A filter run does not create an undo point.
- **Reset**: the history is emptied when a model is loaded or closed.

### Zoom

Frame the visible objects and aim the camera at a picked surface. Both commands work on the **visible** elements, so a fit after an isolation zooms in on the isolated objects.

#### Fit View
On click, the camera zooms to include all the objects of the view.

After positioning, the viewer looks for at least one object within 200 m of the view plane.
If it finds one, nothing more is done.
If it does not, it shows a pop-up: "The objects are far away."

If no object is present in the view, a pop-up shows: "The active view is empty."

#### Perpendicular View
On click, the cursor changes into a set square.
Then, on the first right click, the normal is read.
The view plane must be perpendicular to the normal, in the direction opposite to the normal.
After the view plane is moved, the function deactivates and the cursor returns to its original shape.

## Open Subject

@todo Repair Transparency methdo