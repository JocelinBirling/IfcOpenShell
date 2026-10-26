# BonsaiViewerTI13 — Unique Tree (implementation)

Implements `00_Specification/BonsaiViewer13 Unique Tree.md`.

## Files

| File | Role |
|------|------|
| `modules/unique_tree/Builder.h/.cpp` | Pure logic. Builds the unified tree from plain data (no Qt widgets, no viewport). |
| `modules/unique_tree/DataSource.h/.cpp` | `TreeDataSource` for the TreePanelFramework: levels, rooms, elements; click, double-click, Isolate. |
| `modules/unique_tree/Panel.h/.cpp` | `UniqueTreePanel`: Settings and Run buttons, status line, tree widget. |
| `modules/unique_tree/View.h/.cpp` | `UniqueTreeView`: reads the open models, chooses reference models, runs the builder, shows results. |
| `MainWindow.cpp/.h` | Panel created in `setupPanels` (tabbed with Spatial, hidden at start-up), ribbon toggle "Unified" (DATA group), Window menu entry, view created in `setupLoader`. |
| `CMakeLists.txt` | The eight unique_tree files added to `BONSAIVIEWER_FILES`. |

## Input model (Builder.h)

- `ModelInput`: `session_model_id`, `name`, `storeys`, `spaces`, `elements`.
- `StoreyInput`: `object_id`, `name`, `elevation` (optional, from the `Elevation` attribute).
- `SpaceInput`: `object_id`, `label`, `box` (optional), `storey_object_id` (optional, from `get_container`).
- `ElementInput`: `object_id`, `type`, `label`, `box` (optional).
- `Box`: `mn[3]`, `mx[3]` (world AABB from `ViewportWindow::computeObjectAabb`).

The view fills storeys from the IFC file (`IfcBuildingStorey` instances; name, `get_storey_elevation`), because storeys have no geometry and are not in `ElementRegistry`. Spaces and elements come from `ElementRegistry::basicElementInfoForModel`; a space's storey is `get_container` on its entity (`IfcDataAccess::entity`). Nothing is persisted: the index lives in memory and is rebuilt on each Run.

## Algorithm (Builder.cpp)

1. Validate: reference models set and open; levels model has storeys; rooms model has at least one usable space. Otherwise `UnifiedTree::error` is set and nothing else is built.
2. Levels: one level per storey of the levels model, sorted by elevation (no elevation last). The level tolerance (Settings, default 0.10 m) is passed to `buildUniqueTree` and used by `matchStorey` for the name step and the elevation step.
3. Rooms: each space of the rooms model is placed on the level of its storey (matched through the storey mapping). Spaces without box or storey produce a diagnostic.
4. Slabs (`IfcSlab`): attached to a room when the slab top is at or below the room floor (`top <= room.mn[2] + kFloorTolerance`, `kFloorTolerance = 0.01` m) and the XY footprints overlap. Otherwise the slab is listed under the first level whose elevation is above the slab top, unless it sits below a room.
5. Other elements: attached to a room when their 3D AABB strictly overlaps the room AABB. Walls follow the same rule through their OBB clash (existing AABB is used, as decided).
6. Spatial structure types (`IfcSpace`, `IfcBuildingStorey`, `IfcBuilding`, `IfcSite`, `IfcProject`, `IfcOpeningElement`, `IfcAnnotation`, `IfcGrid`) are not listed as elements.
7. Diagnostics (ambiguous storey names, storey/elevation rejections, rooms without box or storey) are collected in `UnifiedTree::diagnostics`.

## Tree and selection (DataSource.cpp)

- Step 6 of the builder groups levels by site, then by building, with real names. `BuildOptions::merge_sites` / `merge_buildings` (Settings checkboxes, off by default) make the site key or the building key 0 for every level, so all sites (buildings) fall into one node whose name joins the merged names. A merged building is one per site node.
- `BuildOptions::merge_same_name_levels` (Settings checkbox, off) makes `matchStorey` accept a same-name reference level without the elevation check (step 1 `force_by_name`).
- Top level: sites (`site/<s>`). Site children: buildings (`building/<s>/<b>`). Building children: levels. Building children: levels, label `name (x.xx m)` or `(no elevation)` (`level/<index>`).
- Sites, buildings and storeys come from the IFC file (`IfcSite` > `IfcBuilding` > `IfcBuildingStorey` via `get_spatial_children`); storeys are keyed by `spatialKey(session_model_id, ifc_id)` because STEP ids repeat across models.
- Level children: rooms of the level (`room/<object_id>`), then slabs of the level (`elem/<object_id>`).
- Room children: elements (`elem/<object_id>`).
- `idsOf`: level selects its slabs, its rooms and their elements; room selects itself and its elements; element selects itself.
- Click: `SelectionService::setSelection(ids, SelectionSource::Tree)`.
- Double-click and context menu "Isolate": `ActionPipeline::run(ids, Action::Isolate)`.

## Settings and Run (View.cpp)

- Settings: dialog with two combo boxes (levels model, rooms model) listing open models by file name, and a level tolerance spin box (0.01 to 1.00 m, default 0.10 m, kept in `level_tolerance_`).
- Default for both: the open model with the most `IfcSpace` (`modelWithMostSpaces`). A reference that is no longer open is reset to the default.
- Run: `collectModels` → `buildUniqueTree` → status line + `TreePanelWidget::refresh`. Error: status line and popup, empty tree. Diagnostics: status count and popup.

## Open points (not yet validated by the user)

- Levels model default = rooms model default (most spaces).
- Diagnostics shown in the status line and a popup.
- Slab-below-room floor tolerance 0.01 m.
- Non-slab elements outside every room are not shown.
- Only the two reference models are checked for validity.

## Not done

- Not compiled yet. Expect review of includes and `express::base` / `get_container` usage in View.cpp at first build.
- No tests. Verification is by running Run on the test models.
