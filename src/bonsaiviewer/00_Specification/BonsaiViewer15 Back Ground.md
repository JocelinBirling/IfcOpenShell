# BonsaiViewer15 - Back Ground



#DO NOT IMPLEMENT

## Introduction
The idea is to give the user a choice of viewport backgrounds:
- Solid colour
- Vertical gradient
- Procedural sky with clouds

The background choice is a global software setting, saved in the viewer parameters (not per project).

## Specification

### Solid Colour
Already covered: the viewport background colour is already a theme setting ("Viewport Background" in the viewer settings). Nothing to develop.

### Vertical Gradient
The background can be a vertical gradient, from the top of the view to the bottom.
The user freely chooses the two colours of the gradient.

### Procedural Sky
The background can be a procedural sky with clouds, like Blender's Sky Texture (Nishita model). No image file is used; the sky is generated.

## Open Subject
The Nishita parameters exposed to the user (sun position, turbidity, density of clouds) need to be discussed.
This seems quite hard to implemented for a limited use, it's probably easier to go back to blender for a specific background.
