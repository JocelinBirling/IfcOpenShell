# BonsaiViewer13 - Unique Tree

## Introduction
The goal is to link storeys that represent the same level across several models, detect the links between rooms and objects, and build a complete project tree from scratch. The result is a "fictive Spatial Hierarchy" displayed in a dedicated panel alongside the classic one.

## Specification
### Settings and indexing
The Settings declare:

- a reference model for the levels, chosen in a dropdown among the open models;
- a reference model for the rooms, chosen in a dropdown among the open models;
- a level tolerance in metres (default 0.10 m, range 0.01 to 1.00 m), used by the level matching below.

By default, the model with the most spaces is used for the rooms; it will most likely be the model with the most rooms, and therefore the most levels. The same default applies to the levels reference.

The computation is run only with the **Run** button. The index is kept in memory: it is not saved between sessions.

### Hierarchy detection

#### Levels
The reference model is the anchor for level groups. Secondary models are matched to it; levels are not chained or grouped by comparing secondary models with one another. The level tolerance is the value set in Settings.

Apply these matching steps in order:

1. **Exact name:** Match levels whose `IfcBuildingStorey.Name` (`IfcName`) is exactly equal, character by character. Do not normalize case or whitespace. Before accepting a name match, compare the levels' `IfcBuildingStorey.Elevation`; reject and report a match when the elevation difference is greater than the level tolerance.
2. **Elevation:** For levels not matched by name, compare `IfcBuildingStorey.Elevation`. A level within the level tolerance of one or more reference levels is attached to the nearest reference level. If candidates are equally close, attach it to the lower reference level. Do not use OBB geometry to match levels.
3. **Unmatched levels:** If no level is within the level tolerance, attach the level to the nearest reference level when its elevation is between the reference model's lowest and highest elevations. If it is above or below that range, create a new level in the unified tree.

If a model contains duplicate `IfcBuildingStorey.Name` values, do not match those duplicate names automatically; report the ambiguity and use the elevation matching steps for those levels. If `IfcBuildingStorey.Elevation` is missing, do not match that level by elevation and report the omission.

#### Rooms and elements
Room associations are computed with the **axis-aligned bounding boxes** (AABB) already available for the loaded objects. Two boxes match when they overlap. An element is attached to every room whose box it overlaps, so an element may belong to several rooms. Do not run a more precise geometry test after the box test.

- **Walls** and all other objects follow this rule.
- **Slabs** are attached to a room only when the slab is the one below the room (the floor of the room). A slab that is not below the room is not attached to it.

#### Slabs and levels
A slab is also attached to a level: use the slab's top face elevation as its reference elevation, and attach the slab to the first level above that elevation, with no additional tolerance.

### Invalid data and computation
If a model has no usable rooms or levels, or its data is invalid such that the computation cannot be performed reliably, stop the run and show a clear diagnostic. A level missing `IfcBuildingStorey.Elevation` is the exception: skip its elevation match and report the omission.

### Unified tree content
Each unified level holds its rooms; each room holds the elements attached to it:

- Site > Building > Level > Room > Elements (walls and other objects, and slabs below the room).
- Sites and buildings are shown with their real names (`IfcSite.Name`, `IfcBuilding.Name`, the LongName when the Name is empty). Each site contains its buildings, and each building contains its levels.
- Settings has two options, both off by default: "Merge all sites into one" and "Merge all buildings into one". When merged, all the sites (or buildings) form one node named by joining the merged names. Most projects have one site and one building, so the merge options make the tree match them.
- A level without site or building is still listed, under "(no site)" or "(no building)".
- Settings has a third option, "Merge levels with the same name", off by default. When on, a storey whose exact name equals one reference level's name is merged with it without the elevation check. Ambiguous or duplicated names are still reported and not merged.
- Sites, buildings and storeys are read from the IFC file: they have no geometry, so they are not in the element registry.
- Slabs attached to the level only (not below a room) are listed directly under the level.

### Display
Show two separate panels: one with the classic Spatial Hierarchy and one with the Unified Spatial Hierarchy. The unified panel uses the same general panel design as today, but displays the more complete tree.

A Settings button opens the choice of the two reference models (levels and rooms) and the level tolerance. A Run button launches the computation. The panel has these two buttons.

## Open Subject
- Definition of "the slab below the room" for the implementation: a slab whose top face is at or below the room's floor elevation and whose box overlaps the room's box. To be confirmed when the code is written.
- Diagnostics (duplicate names, missing elevations, invalid data): shown where, and how (status bar, dialog, or list in the panel). To be decided.
