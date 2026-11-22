<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer43 - SVG Plan Generation

Status: draft, to validate

## Introduction

The **Map chart** of BonsaiViewer25 displays an SVG plan whose elements are linked to IFC objects by their GlobalId, but it only loads SVGs provided by the user. This spec covers the **generation** of those SVG files from the loaded models: one SVG per building storey — a horizontal **section** at a configurable **cut height**, with the spaces as shapes in phase 1 and the other objects (walls, slabs…) in phase 2 — so the user gets a usable starting point and can then edit the SVG by hand. Auto-generation is only a starting point; manual editing stays the durable path (BonsaiViewer25, Map chart).

**The generation reuses the SVG serializer already shipped in IfcOpenShell** — `ifcopenshell::serializers::svg_serializer` (`src/serializers/svg_serializer.{h,cpp}`; Python: `ifcopenshell.geom.serializers.svg`, high-level `ifcopenshell.draw` in `src/ifcopenshell-python/ifcopenshell/draw.py`), already used by the Bonsai drawing module (`src/bonsai/bonsai/bim/module/drawing/operator.py`, `src/bonsai/scripts/standalone_drawer.py`). BonsaiViewer does **not** re-implement slicing or filling: it drives the existing serializer with the right settings (see Reused engine).

Generated SVGs are ordinary **2D Plan documents** of the External Documents tree (BonsaiViewer35): they can be shown as a background of the 3D view (BonsaiViewer36, phase 2) and used in a Map chart (BonsaiViewer25, phase 2).

Related specs: BonsaiViewer02 Display Function, BonsaiViewer21 Add Panels, BonsaiViewer25 Chart Viewer, BonsaiViewer27 CtrlZ, BonsaiViewer31 Keybindings, BonsaiViewer35 External Table, BonsaiViewer36 Add Drawing Background.

## Specification

### Where the feature lives (no new tab)

- No new ribbon tab. The feature lives in the **External Documents** panel (BonsaiViewer35), docked **bottom left of the Table tab**, next to the Tables panel, and addable to any user tab from the panel catalogue (BonsaiViewer21).
- **Empty state** of the panel is the one of BonsaiViewer35; the **Generate SVG** button (below) is **disabled when no model is loaded**.

### Entry points

- Toolbar button **Generate SVG** of the External Documents panel: opens the SVG Generation window. **Disabled when no model is loaded.**
- Right-click on the **root or a folder** of the tree: the existing menu of BonsaiViewer35 (Add Folder, Import Document, Save / Load Documents (JSON), Rename, Delete) gains one item — **Generate SVG Plans…**, which opens the same window, generating into that folder.

### Reused engine — mapping to the existing SVG serializer

| BonsaiViewer43 concept | Existing support in `svg_serializer` / geom settings |
|---|---|
| Cut plane at *storey elevation + cut height* | `section-height-from-storeys = true` + `section-height = <cut height>` (`SvgSectionHeightFromStoreys` / `SvgSectionHeight`, `setSectionHeightsFromStoreys(offset)`; library default offset **1.2 m**). One `<g class="section">` per storey |
| Cut fill (the « SVG fill ») | Elements cut by the plane get `class="cut <IfcClass>"` with CSS `fill: #444444; fill-rule: evenodd`; the others are `class="projection"` with `fill: none; fill-opacity: .2` |
| GlobalId link for the Map chart | `use-element-guids = true` → element `id` = GlobalId; every `<g>` also always carries `ifc:guid` = GlobalId and `ifc:name` = Name |
| Space labels at the centre of each shape | `print-space-names = true` (and `print-space-areas = true`) → `<text text-anchor="middle">` per IfcSpace; for other classes the Map chart reads `ifc:name` (BonsaiViewer25) |
| Classes to draw (spaces, walls…) | Geometry iterator `include` / `exclude` by IFC class |
| One file per storey | One serializer run per storey, iterator `include` = the elements of that storey, `svg-without-storeys = true` |
| Merge several storeys in one file (phase 2) | One run, all storeys as `<g class="section">` sections in a single SVG |
| Reflected plan view (phase 2) | `svg-mirror-y = true` |

### SVG Generation window (edit view)

Modal window opened by **Generate SVG** / **Generate SVG Plans…**:

- **Models** — dropdown: **All loaded models** (default, the scope rule below) or one specific loaded model. **Disabled (forced to "All loaded models") when only one model is loaded.**
- **Storeys** — checklist of the IfcBuildingStorey of the selected scope, one row per storey (model name + storey name); default: all checked. A storey without any drawable object is listed greyed and skipped.
- **Cut height** — number in meters, default `1.2` (same default as the serializer's `setSectionHeightsFromStoreys`). The **cut plane** is the horizontal plane at *storey elevation + cut height*; mapped to `section-height-from-storeys` + `section-height`.
- **Classes** — multi-select list of the IFC classes present in the selected scope (IfcSpace, IfcWall, IfcSlab, IfcColumn…), plus an **Add class…** input with completion on all the classes of the loaded models. Default: **IfcSpace** checked only; the other classes are enabled from phase 2. Mapped to the iterator's `include` / `exclude`.
- **Output folder** — text field + **Browse…** (folder dialog). Default: the folder of the first loaded model file. Required.
- **File naming** — automatic: `<StoreyName>.svg`, sanitized (no path separators); if the name is empty, the storey GlobalId is used. Merged output (phase 2) uses a **File name** field instead.
- Validation: **OK** is disabled when no storey is checked, no class is checked, or the output folder is empty. **OK** runs the generation, adds the documents to the tree, shows the SVG Generation Report and closes; **Cancel** discards everything.

### Generation rules

- For each checked storey, the existing serializer produces one `.svg` file in the output folder: one geometry-iterator run restricted to the elements of that storey belonging to the checked classes, with the Bonsai drawer's tuning (`svg-without-storeys`, `svg-write-poly`, `svg-poly`, `svg-project`, embedded CSS so the file is standalone and hand-editable).
- The serializer classifies every element against the cut plane: **cut** elements (spaces, walls…) are filled shapes (`class="cut IfcWall"`), the rest are **projection** outlines (`class="projection"`).
- Every element group is `<g id="<GlobalId>" class="<IfcClass>" ifc:name="…" ifc:guid="…">` (`use-element-guids = true`) — the link used by the Map chart of BonsaiViewer25, for any class.
- Shapes are drawn in the storey's XY plane, in meters, with an automatic `viewBox`; Y is not flipped by default (`svg-mirror-y` for reflected views).
- **Height Z of the 3D background**: when the document is added to the tree, BonsaiViewer sets the plan configuration **Height (Z)** = storey elevation + cut height (BonsaiViewer36, Plan Configuration window). The SVG file itself is left untouched — pure serializer output, no custom metadata — so it stays hand-editable and reusable outside BonsaiViewer.
- **Overwrite rule**: if the target file already exists, a small dialog asks **Overwrite** / **Keep both** (the new file gets `_1`, `_2`… appended). BonsaiViewer never overwrites an existing SVG silently — a hand-edited file is never lost.
- Elements without a geometry representation are skipped by the iterator and listed in the report.

### Result in the External Documents tree

- Each generated `.svg` file becomes a **2D Plan document** in the tree (BonsaiViewer35), in the folder where the action was triggered (or at the root), named after its file, state **loaded**, with its plan **Height (Z)** preset from the generation (storey elevation + cut height).
- The document keeps the standard plan behavior: right-click menu of BonsaiViewer36 (**Show / Hide**, **Edit Plan**, **Refresh**, **Locate File…**, **Export / Import Config (JSON)**, **Delete**); phase 2 adds **Open in Map Chart** — creates a Map chart in the Dashboard view with this SVG (BonsaiViewer25).
- Click behaviors on the tree nodes are the ones of BonsaiViewer35 / BonsaiViewer36:
  - **document node** — single left-click: select / quick menu (BonsaiViewer36); double left-click: show the plan in the 3D view; right-click: full context menu above;
  - **folder node** — single left-click: select; double left-click: expand / collapse; right-click: **Add Folder**, **Import Document**, **Generate SVG Plans…**, **Rename**, **Delete**, **Save / Load Documents (JSON)**;
  - **root / empty space** — single left-click: deselect; double left-click: no action; right-click: **Add Folder**, **Import Document**, **Generate SVG Plans…**, **Save Documents (JSON)**, **Load Documents (JSON)**.

### SVG Generation Report

- Shown after each generation: the list of written files (path, storey, cut height) and the list of **skipped objects** (checked classes / selected storeys that produced no shape; reason: no geometry), with click-to-select in the 3D view on the object rows, like the Match report of BonsaiViewer35.
- The status bar shows the summary: "12 SVG plan(s) generated in `<folder>`" or the error (no model loaded, output folder not writable, no drawable object in the selected storeys).

### Scope

- Default scope of the generation: **all the loaded models**; the Models dropdown of the window restricts it to one loaded model for a single run (not saved).
- The generation **ignores the current box** (BonsaiViewer04): the cut height is an absolute height per storey, not the box top.

### Save / load configuration

- The **generated documents are saved with the project** like every external document: tree entry, file path, plan configuration including Height (Z) (BonsaiViewer35, BonsaiViewer36), reloaded at start-up; JSON export / import per BonsaiViewer35 / BonsaiViewer36.
- The **generation options** (storeys checked, cut height, classes, output folder) are **session only, nothing is saved**.
- The SVG files themselves are files on disk; their content is not embedded in the project.

### Transversal behaviors

- **Undo** (BonsaiViewer02 / BonsaiViewer27): generating adds documents to the tree — one CTRL+Z point removes all the documents created by the run. Writing / overwriting a file on disk is not an undoable action.
- **Selection sync**: generation selects nothing; the report rows and the Map chart (BonsaiViewer25) select / isolate objects in the 3D view.
- **Status bar**: progress during generation ("Storey 3 / 12…"), result summary, error messages (see Report).
- **Empty state**: no model loaded → **Generate SVG** disabled; a run with every storey empty → error message "No drawable object in the selected storeys" and no file written.
- **Keyboard shortcut**: none in v1 (BonsaiViewer31).
- **Drag & drop**: none added; documents are moved in the tree by drag & drop as in BonsaiViewer35.

### Delivery phases

- **Phase 1**: one SVG per storey, IfcSpace only, **Cut height** field, generation window, report, tree integration, Height (Z) preset, save with the project.
- **Phase 2**: **classes other than IfcSpace** (IfcWall, IfcSlab, IfcColumn…) through the Classes field, merge several storeys into one SVG (the serializer's multi-section output), **Open in Map Chart** on generated documents, reflected plan option (`svg-mirror-y`), drawing frame (north arrow, scale bar).

## Performance

- The serializer runs HLR on the OpenCASCADE geometry kernel; heavy on large models: generation runs on demand only, geometry read once per run, progress bar per storey in the status bar.
- Reuse the tuning settings of the Bonsai drawing module (`profile-threshold`, `svg-prefilter`, `svg-unify-inputs`, `svg-segment-projection`, `svg-subtract-before`).

## Decisions

- Decided (2026-11-22): the auto-generation of the SVG plans is a separate spec (this one, BonsaiViewer43); BonsaiViewer25 only loads user-provided or generated SVGs. Generated SVGs are ordinary 2D Plan documents of the External Documents tree (BonsaiViewer35), linked to the objects by `id` = GlobalId.
- Decided (2026-11-22): the generation **reuses the existing IfcOpenShell SVG serializer** (`src/serializers/svg_serializer.cpp`; Python `ifcopenshell.geom.serializers.svg` / `ifcopenshell.draw`, already used by the Bonsai drawing module) — cut plane via `section-height-from-storeys` + `section-height` (library default offset 1.2 m), cut fill via `class="cut <IfcClass>"`, GlobalId link via `use-element-guids` + `ifc:guid`. No custom slicing / fill code in BonsaiViewer.
- Decided (2026-11-22): the cut plane is at *storey elevation + cut height* (default 1.2 m); the plan's Height (Z) in the 3D background is set by BonsaiViewer from these values at document creation — the SVG file itself carries no custom metadata. The generation ignores the current box (BonsaiViewer04).
- Decided (2026-11-22): walls and the other objects appear as the serializer's cut / projection shapes, one group per object, `id` = GlobalId; the Map chart of BonsaiViewer25 links any of them, not only spaces.

## Open Subject

- Phase 2 merge: one SVG with all storeys as `<g class="section">` sections (the serializer's native output) vs several files — confirm the merged file keeps a per-storey offset readable by the Map chart.
- Duplicate overlapping spaces and minimum-area filtering: the serializer subtracts overlaps (`svg-subtract-before`) but has no per-element area threshold — post-filter the SVG, or drop the option.
- Per-class colors: the serializer's embedded CSS fills all `cut` shapes with one color — a per-class palette needs a post-processing pass or a hand-edited CSS.
- Drawing frame (north arrow, scale bar) in phase 2: the serializer has a `scale` setting for annotations — decide whether a frame is added by BonsaiViewer or left to hand editing.
