# BonsaiViewer19 - Display Drawings

**This can not be deployed, as is, and need a lot of work.** 

## Introduction
Each file can have zero to many drawings in it. The idea is to display them in the viewer.
A drawing is the 2D vector drawing contained in the IFC, Bonsai style (IfcAnnotation). No imported raster image (PNG, PDF, SVG).

## Specification

### Drawings as Built-in Levels
Yes: a drawing is displayed as a built-in level. Each drawing sits at its own elevation in the 3D view, stacked in the same view as the model.

### Dedicated 2D View
A drawing can also be opened alone, flat, in a dedicated 2D view.
The 2D view and the 3D view are synchronised: selecting a drawing element in one view selects it in the other.

### Drawings Management
- **Hide and display drawings** - per drawing.
- **Colour drawings** - per drawing.
- **Transparency of drawings** - per drawing.

The settings (colour, transparency, visibility) are applied to the drawing as a whole.

### Selection
A click on a drawing element selects the 3D object it documents.
The click targets the individual element (a line, a text), the display settings stay per drawing.

## Out of Scope
- Displaying imported raster images (PNG, PDF, SVG) as drawings.

## Open Subject
How to manage architectural drawings and structural engineer drawings (top-to-bottom view and bottom-to-top view): how the discipline and the view direction are recognised and presented needs to be discussed.
