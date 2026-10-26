# BonsaiViewer13 - Unique Tree

## Introduction
The goal is to link storeys that represent the same level across several models, detect the links between spaces and objects, and build a complete project tree from scratch. The result is a "fictive Spatial Hierarchy" displayed in a dedicated panel alongside the classic one.

## Specification
### Settings and indexing
Two methods can be used, declared in the settings:

- Declare a reference model for the levels.
- Declare a reference model for the rooms.

By default, the model with the most spaces is used; it will most likely be the model with the most rooms, and therefore the most levels.

When a model is added, the rule must be rerun. If the settings have not changed, only new or modified models are indexed. Changes to an already indexed model must be detected so that it can be reindexed without reprocessing unchanged models.

### Hierarchy detection

#### Levels
The reference model is the anchor for level groups. Secondary models are matched to it; levels are not chained or grouped by comparing secondary models with one another.

Apply these matching steps in order:

1. **Exact name:** Match levels whose `IfcBuildingStorey.Name` (`IfcName`) is exactly equal, character by character. Do not normalize case or whitespace. Before accepting a name match, compare the levels' `IfcBuildingStorey.Elevation`; reject and report a match when the elevation difference is greater than 0.10 m.
2. **Elevation:** For levels not matched by name, compare `IfcBuildingStorey.Elevation`. A level within 0.10 m of one or more reference levels is attached to the nearest reference level. If candidates are equally close, attach it to the lower reference level. Do not use OBB geometry to match levels.
3. **Unmatched levels:** If no level is within 0.10 m, attach the level to the nearest reference level when its elevation is between the reference model's lowest and highest elevations. If it is above or below that range, create a new level in the unified tree.

If a model contains duplicate `IfcBuildingStorey.Name` values, do not match those duplicate names automatically; report the ambiguity and use the elevation matching steps for those levels. If `IfcBuildingStorey.Elevation` is missing, do not match that level by elevation and report the omission.

#### Rooms and elements
Detect room/space associations using OBB clash computation only. Attach an element to every room or space whose OBB matches; elements may therefore be attached to multiple rooms or spaces. Do not run a more precise geometry test after the OBB test.

#### Slabs
Use the slab's top face elevation as its reference elevation. Attach the slab to the first level above that elevation, with no additional tolerance.

### Invalid data and computation
If a model has no usable rooms or levels, or its data is invalid such that the computation cannot be performed reliably, stop the run and show a clear diagnostic. A level missing `IfcBuildingStorey.Elevation` is the exception: skip its elevation match and report the omission.

### Display
Show two separate panels: one with the classic Spatial Hierarchy and one with the Unified Spatial Hierarchy. The unified panel uses the same general panel design as today, but displays the more complete tree.

A Settings button allows selecting among the open models in a dropdown; a Run button launches the computation. The panel has these two buttons.

## Open Subject
