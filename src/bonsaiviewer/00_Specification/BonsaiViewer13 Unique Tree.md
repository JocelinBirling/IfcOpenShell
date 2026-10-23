# BonsaiViewer13 - Unique Tree

## Introduction
The goal is to link all the storeys that should be linked across several models, and to detect the links between spaces and objects in order to build a tree from scratch. The result is a "fictive Spatial Hierarchy" but complete project tree, displayed in a dedicated panel alongside the classic one.

## Specification
### Settings
Two methods can be used, declared in the settings:

- Declare a reference model for the levels.
- Declare a reference model for the rooms.

By default, the model with the most spaces is used; it will most likely be the model with the most rooms, and therefore the most levels. 

When a model is added, the rule must be rerun. If the settings have not changed, only the new models are indexed.
### Hierarchy detection

#### Rooms
A simple clash computation for the rooms, using OBBs to ease the calculation. If the computation passes with the OBB, stop there. If several results are found, continue.
Elements can be attached to several rooms and spaces.

#### Levels
The reference model gives the position intervals of the rooms. The position of the elements then gives their level attachment.

Slabs, which are often below their level, follow a specific rule: they are attached to the level above them (a slab just below level 1 is attached to level 1).
### Display
Two separate panels: one with the classic Spatial Hierarchy, one with the Unified Spatial Hierarchy.
It's the same panel as today, but with a more "complete" tree.

A Settings button allows selecting among the open models in a dropdown; a Run button launches the computation. The panel has these two buttons.

## Open Subject
