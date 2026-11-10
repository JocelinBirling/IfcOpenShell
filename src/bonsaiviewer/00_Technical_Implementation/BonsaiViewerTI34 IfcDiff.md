<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI34 - IfcDiff

Spec source: `../00_Specification/BonsaiViewer34 IfcDiff.md`
Engine source: `src/ifcdiff/ifcdiff.py` (Python only — class `IfcDiff`)

## What exists today

- `src/ifcdiff/ifcdiff.py` — the diff engine, Python only. `IfcDiff(old, new,
  relationships, is_shallow, filter_elements)` builds `added_elements`,
  `deleted_elements` and `change_register` (GlobalId → changes), then
  `export(path)` writes `{"added": [...], "deleted": [...], "changed": {...}}`.
  Relationships: `attributes`, `geometry`, `type`, `property`, `container`,
  `aggregate`, `classification`.
- BonsaiViewer: no diff module, no embedded Python. Geometry is already
  produced per model by the C++ IfcGeom pipeline (SceneLoader, `.ifcview`
  cache), and placements are computed for rendering.

## Decision: C++ port, no subprocess

The viewer is pure C++/Qt (its CMakeLists has no Python). The engine is
ported to C++ inside `src/bonsaiviewer` (`modules/diff/` for the UI,
`services/IfcDiff/` for the engine), reusing the viewer's IfcGeom/IfcParse
C++ APIs. The exported JSON keeps the Python schema so existing consumers
(CLI diff.json) stay compatible.

## Building blocks used

- B01 SelectionService — the "select by change kind" buttons.
- B04 AppearanceService — diff colours as a new priority layer (above clash),
  with original-colour preservation and reset on model reload.
- B05 IfcDataAccess + PropertyIndex — attribute / pset reads for the
  `attributes` and `property` relationships and the Properties panel colouring.
- B13 PanelManager — the system "Diff" tab and the IfcDiff panel layout.
- B14 PersistenceService — Export JSON of the diff result.
- B17 PopupService — confirmations and empty-run messages.
- `SessionState` — progress bar during the run, undo states (depth 10).
- `ElementRegistry` — object id ↔ entity lookup per model.

## New code, by phase

### Phase 1 — engine port: added / deleted / attributes / relationships

Port of `IfcDiff.diff()` minus geometry:

- Element universe: `IfcElement` + `IfcSpatialStructureElement` (IFC2X3; the
  viewer builds with `--schemas 2x3` only, see `agend.md`) minus
  `IfcFeatureElement`; GlobalId sets → added / deleted / common.
- `attributes`: compare all attributes except entity instances and tuples,
  with `math_epsilon = precision` (from
  `IfcGeometricRepresentationContext.Precision`, default 1e-4 — answers the
  open subject of spec 34), ignoring string/numeric type changes →
  `attributes_changed: true`.
- Relationships `type`, `property`, `container`, `aggregate`,
  `classification`: same logic as `diff_element_relationships()` →
  `type_changed` / `properties_changed` (with the DeepDiff detail kept) /
  `container_changed` / `aggregate_changed` / `classification_changed`.
- `filter_elements` → IDS shortlist hook (phase 4); `is_shallow` kept.
- Export `{"added", "deleted", "changed"}` — identical schema to Python.

### Phase 2 — geometry diff, with exported details (see below)

Port of `summarise_shapes()`, `diff_element_basic_geometry()` and
`diff_representation()` / `diff_representation_item()`, plus the **detailed
export** that the Python tool does not do.

### Phase 3 — Diff tab, panel and run

- System tab "Diff" (B13, like spec 34): Models top left, IfcDiff bottom
  left, Properties right, 3D view center; reachable via Window > Diff.
- IfcDiff panel: Before / After dropdowns (all loaded models), relationship
  checkboxes (default attributes, geometry, property), `is_shallow` toggle,
  IDS shortlist picker, Run with progress bar, Export JSON (B14).
- Results stored in the panel: added / deleted GlobalIds, changed GlobalId →
  change kinds + geometry details.

### Phase 4 — visualisation

- Viewport colours via B04 (red deleted / green added / blue-orange modified,
  per spec 34), transparency toggle, Clear Diff, undo states, clearing on
  model load/remove.
- Selection buttons by change kind (B01), report window reusing the table
  widget of TI07, property field (BonsaiViewer26), Properties panel row
  colouring (green / red / orange with old → new tooltip).

## Geometry modifications: computed AND exported in the C++ port

The Python engine already computes per-element geometry details but exports
only `"geometry_changed": true`. The C++ port must keep the boolean (schema
compatibility) and add a `geometry` detail object per changed element.

### What the Python engine computes today (source: `ifcdiff.py`)

| Detail | Computed in | Exported today |
|---|---|---|
| `total_verts` — length of the flat vertex array | `summarise_shapes()` | no |
| `sum_verts` — sum of all vertex floats | `summarise_shapes()` | no |
| `min_vert` / `max_vert` — min / max over the flat vertex array (scalars) | `summarise_shapes()` | no |
| `matrix` — shape transformation matrix (kernel) | `summarise_shapes()` | no |
| `openings` — sorted GlobalIds of `HasOpenings[].RelatedOpeningElement` | `summarise_shapes()`, `diff_element_basic_geometry()` | no |
| `projections` — sorted GlobalIds of `HasProjections[].RelatedFeatureElement` | same | no |
| Translation delta — placement column `[:,3]`, `atol = precision` | `diff_element_basic_geometry()` | no |
| Rotation delta — placement 3×3 part, `atol = 1e-2` | `diff_element_basic_geometry()` | no |
| Per-item representation diff — DeepDiff of `get_info(recursive=True)`, excluding `.*id']$` | `diff_representation_item()` | no |
| Shape comparison — DeepDiff of the summaries, `math_epsilon = 1e-5` | `diff()` | only `geometry_changed: true` |

### Export schema (addition, backward compatible)

```json
"changed": {
  "<GlobalId>": {
    "geometry_changed": true,
    "geometry": {
      "placement": {
        "translation_old": [x, y, z],
        "translation_new": [x, y, z],
        "translation_delta": [dx, dy, dz],
        "rotation_changed": true
      },
      "shape": {
        "old": {"total_verts": 384, "sum_verts": 12.34, "min_vert": -1.5,
                "max_vert": 8.2, "matrix": [/* kernel matrix */],
                "openings": ["guid..."], "projections": ["guid..."]},
        "new": {"...": "same fields, or null when the element has no geometry"},
        "fields_changed": ["sum_verts", "matrix", "openings"]
      },
      "openings_added": ["guid..."],
      "openings_removed": ["guid..."],
      "projections_added": ["guid..."],
      "projections_removed": ["guid..."],
      "representation": {
        "representation_id_old": 123,
        "representation_id_new": 456,
        "items_changed": [
          {"index": 2, "type_old": "IfcExtrudedAreaSolid", "type_new": "IfcFacetedBrep",
           "attributes": {"Depth": {"old": 0.2, "new": 0.3}}}
        ]
      }
    }
  }
}
```

### C++ implementation notes for the geometry details

- Geometry source: run the C++ IfcGeom iterator **only on the potential
  changes** (elements with a `Body` / `MODEL_VIEW` representation), with the
  same settings as `get_settings()`: `disable-boolean-result`,
  `disable-opening-subtractions`, `context-ids` = sub-contexts
  `Body`/`Facetation` + top-level `Model` contexts. Alternatively reuse the
  viewer's already-loaded geometry (`.ifcview`) — technical question below.
- `verts`: flat `std::vector<double>` from the triangulation; `total_verts`,
  `sum_verts`, `min_vert`, `max_vert` computed in double, same semantics as
  Python (flat array, scalars) so results are comparable across tools.
  Additionally export `vertex_count = total_verts / 3` for readability.
- `matrix`: the shape transformation matrix from the kernel (`IfcGeom::Matrix`),
  exported as a flat array; compare element-wise with `math_epsilon = 1e-5`.
- Placement: decompose the `ObjectPlacement` chain into a 4×4 (reuse the
  viewer's placement code used for rendering); compare the translation column
  with `atol = precision` and the 3×3 rotation part with `atol = 1e-2`,
  exactly like `diff_element_basic_geometry()`; export old / new / delta.
- Openings / projections: inverse relationships `HasOpenings` /
  `HasProjections` via IfcParse; collect `RelatedOpeningElement` /
  `RelatedFeatureElement` GlobalIds, sort, export added / removed sets.
- Representation items: `IfcShapeRepresentation` with
  `RepresentationIdentifier == "Body"`; for `MappedRepresentation` follow
  `MappingSource.MappedRepresentation` (as `get_representation_id()` does).
  Compare items pairwise, recursively over the express attributes (excluding
  id attributes, as `.*id']$`), numbers with epsilon; export per-item type and
  attribute-level old/new diffs as above. `is_shallow` stops at the first
  differing item (DiffTerminator behaviour).
- Shape summary comparison: field-by-field with `math_epsilon = 1e-5`, and
  record `fields_changed` with old/new values — this is the detail the
  Python tool computes but drops.

## Key flows

Run → collect GlobalId sets → attributes + relationships diff → queue
elements with a Body representation → geometry summaries (old / new) →
placement + openings + representation items → change_register with geometry
details → B04 colours + panel state → report window / Export JSON. Re-run
replaces the result and clears previous colours first (undo point stored).

## Technical questions — decisions taken (v1)

- Geometry source: **neither the kernel nor the `.ifcview` geometry**. The
  geometry is compared on the IFC data itself: placement (`get_local_placement`
  of `src/helpers/placement.h`), openings, projections, and the items of the
  Body `IfcShapeRepresentation`, compared recursively attribute by attribute
  (Python's "Option 3"; slow in Python, fast in C++ with a memo on
  `(old id, new id)` pairs, shared across the run so a type's mapped
  representation is compared once). No tessellation, so no `shape` summary
  (`total_verts`, `sum_verts`, `matrix`) in the export; in exchange every
  item change has its attribute path and old / new values.
- Matrix layout: no kernel matrix is exported; the placement is exported as
  translation old / new / delta and `rotation_changed`.
- Epsilons: attributes, properties, translation and representation numbers
  use the model `Precision` (first `Model` context of the After file, default
  1e-4); the rotation part uses 1e-2, as `diff_element_basic_geometry()`.
- Parallelism: none. The run is on the GUI thread (the IFC files are shared
  with the panels); the progress bar is repainted during the run, user input
  is held. The 973-element Duplex MEP model diffs in about 1 s.
- `is_shallow`: off by default in the panel (the property filters need every
  change). On: the first differing relationship per element, the first
  differing item per representation (DiffTerminator behaviour).
- Spec 34 open subjects: precision = model `Precision`; attribute-only
  changes count as "modified" (the `attributes` relationship is a checkbox);
  the report is a non-modal window; export = IfcDiff JSON, plus the CSV of
  the report table.

## Implementation (v1)

| Part | Files |
|---|---|
| Engine (port of `IfcDiff.diff()`, JSON export) | `services/IfcDiff/IfcDiff.h/.cpp` |
| Property changes for the Properties panel | `services/IfcDiff/DiffMarks.h` |
| IfcDiff panel widget | `modules/diff/Panel.h/.cpp` |
| Run, colours, undo, selection, report | `modules/diff/View.h/.cpp` |
| Report table and Before / After comparison | `modules/diff/ReportDialog.h/.cpp` |
| Diff tab, Window > Diff, panel wiring | `MainWindow.cpp`, `modules/menubar/MenuBar.*` |
| Properties panel colours | `modules/properties/{Types.h,View.cpp,Panel.cpp}` |
| Diff colour layer | `services/AppearanceService.h` (`Layer::DiffResult`) |

Notes:

- Diff colours are a layer of B04 placed after the user paint: a Paint on a
  diffed element still wins; the diff wins over table, IDS, filter colours.
- A run stores one undo state, then replaces the previous colours; Clear
  Diff and Swap transparency store one each. A model loaded or removed drops
  the result and its colours without an undo state.
- Container / aggregate are compared by GlobalId (Python compares instances
  of two files, which never match); with `is_shallow` off every relationship
  is checked (Python stops at the first different one either way).
- `properties_changed` is exported in the DeepDiff layout (`values_changed`,
  `type_changes` for a value added / removed, `dictionary_item_added`,
  `dictionary_item_removed`, with `root['Pset']['Property']` paths);
  `attributes` and `geometry` detail objects sit next to the booleans.
- The IDS shortlist: each specification is checked against the IFC version
  of each file (`check_ifc_version`), its applicable elements of either model
  are compared on the property requirements of the specification only.
- Geometry selection buttons (beside the property ones): Select / Isolate
  the elements, in both models, with any geometry change, a move
  (`placement.moved`, also exported), a rotation, a Body shape change, or
  openings / projections added or removed.
- Not done in v1: the property field proposes all the properties of the
  loaded models (PropertyCatalog), not only those of the IDS shortlist.
