# BonsaiViewer13 - Unique Tree

## Introduction
The goal is to link storeys that represent the same level across several models, detect the links between rooms and objects, and build a complete project tree from scratch. The result is a "fictive Spatial Hierarchy" displayed in a dedicated panel alongside the classic one.

## Specification
### Settings and indexing
The Settings declare:

- a reference model for the levels, chosen in a dropdown among the open models;
- a reference model for the rooms, chosen in a dropdown among the open models;
- a level tolerance in metres (default 0.10 m, range 0.01 to 1.00 m), used by the level matching below;
- "One level per element", on by default (see Rooms and elements).

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
Room associations use the **real shape of the room in plan** (its triangles projected on the floor), with a tolerance of **5 cm**: the test does not need to be more precise. The axis-aligned bounding boxes only pre-select the candidate rooms. (Decision of October 2026: the box test alone gave far too many matches, with L-shaped or rotated rooms, touching boxes and long elements.)

- **Ordinary elements**: the centre of the element's box must be inside the room's shape in plan (5 cm tolerance) and within the room's height. An element therefore belongs to one room, rarely more. A long element (duct, pipe, beam) goes to the room that holds its centre.
- **Walls and columns** (`IfcWall*`, `IfcCurtainWall`, `IfcColumn`): attached by contact: the wall's shape in plan touches the room's shape (5 cm tolerance) and the wall shares more than 5 cm of height with the room. A wall between two rooms belongs to both; a wall of the storey above (on the slab) does not.
- **Slabs** are attached to a room only when the slab is the one below the room (the floor of the room): the top face of the slab is within the level tolerance of the room floor, and the slab's shape in plan meets the room's shape (5 cm tolerance). A slab that is not below the room is not attached to it.
- **Missing shape**: when the mesh of a room or of a wall / slab is not loaded, the test falls back on the boxes; the number of such tests is reported in the diagnostics.
- **Rooms** are the `IfcSpace` of the rooms reference model only. The `IfcSpace` of the other models are not rooms: they are handled as ordinary elements.
- **One level per element** (Settings, on by default): when the rooms an element overlaps are on several levels, only the rooms of one level are kept: the level of the element's IFC storey when one of its rooms is there, otherwise the level where its box shares the most height with the rooms (ties: the lower level). The tooltip says which level was kept and why. Off, an element is listed under every room it overlaps, whatever the level.
- **Several rooms**: an element that overlaps several rooms is listed under each of them, with a mark giving the number of rooms (for example "×3"). The counts of a level or a building count such an element once.
- **No room**: an element that overlaps no room is listed under its level, in a "(no room)" node. Its level is the unified level of the `IfcBuildingStorey` that contains it in its own model.

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

### Interpreted data
The Unified Tree is an interpretation of the models, not data read from them. This must always be visible:

- The panel is titled "Unified Tree (interpreted)", and a banner at the top says: "Deduced data: not read from the models."
- Every deduced attachment carries an icon; its tooltip gives the reason: level matched by name, by elevation, or attached to the nearest level; element attached to a room by box overlap; slab attached to a level by its top face; element placed under "(no room)" from its IFC storey.
- Sites, buildings, storeys and rooms keep their real names, read from the IFC.

### Display
Show two separate panels: one with the classic Spatial Hierarchy and one with the Unified Spatial Hierarchy. The Spatial Hierarchy and the IFC Class Tree (BonsaiViewer12) stay as they are: the Unified Tree is an additional panel. The unified panel uses the same general panel design as today, but displays the more complete tree.

A Settings button opens the choice of the two reference models (levels and rooms) and the level tolerance. A Run button launches the computation. The panel has these two buttons.

### Implementation rules
Choices made when the first version was written:

- **Elevations**: `IfcBuildingStorey.Elevation` converted to metres with the model's length unit, then placed in the federation (false origin, model transformation, coordinate operation), so that levels of models placed differently can be compared.
- **Shapes in plan**: read from the meshes already loaded for display (CPU copy of the geometry), projected on the XY plane; vertical faces are dropped.
- **Slab without room**: attached to the first level whose elevation is at or above its top face; with no level above, it goes to "(no level)".
- **Room or element without storey**: listed in a "(no level)" node at the root.
- **Merge all buildings**: one building node per site.
- **Diagnostics**: a "Diagnostics" folder at the bottom of the tree lists every message; the status line of the panel gives the counts (levels, rooms, elements, diagnostics).
- **Run**: needs the IFC data of every open model; otherwise the run stops and says which model is not open yet. The settings stay in memory for the session; the reference models default to the model with the most `IfcSpace`.
- **Panel**: tabbed with the Spatial Hierarchy, hidden by default; shown from the Window menu or the Panels ribbon ("Unified Tree").
- **Interactions**: click selects the node and everything below, double-click isolates, right click: Isolate, Select, Show properties (sites, buildings, levels, rooms and elements, BonsaiViewer14).

## Open Subject


- Manual editing (later version): once created, allow the user to change the tree: move an element, remove one, check which ones are out, merge two nodes. If the computation creates 80% of the tree, editing eases the cleaning of the rest. Needs a storage for the corrections (for example in the .ifcfed); the first version is read-only.
