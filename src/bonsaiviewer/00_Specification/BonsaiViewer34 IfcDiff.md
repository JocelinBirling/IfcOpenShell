# BonsaiViewer34 - IfcDiff

## Introduction

The goal is to compare two IFC models already open in the viewer — a **Before** model and an **After** model — and highlight the differences directly in the 3D view and in the Properties panel, using the IfcDiff engine (`src/ifcdiff`).

The comparison runs from a dedicated **Diff** workspace tab, which holds an **IfcDiff panel**, the Models panel, the Properties panel and the 3D view. The panel lists the loaded models, lets the user pick the Before and After models, runs the comparison, and exposes the result through colours in the viewport, selection buttons, a report table, and colour coding in the Properties panel.

The comparison covers two aspects, which can be run together or separately:

- **Geometry and structure**: added, deleted and modified elements (attributes, geometry, type, container, aggregation, classification);
- **Properties**: added, deleted, modified property sets and values, optionally restricted by an IDS shortlist.

## Specification

### Diff workspace tab

- A new **system tab "Diff"** in the ribbon (BonsaiViewer21), next to Home / Filter / Table and before Inspect. Like the other system tabs, it cannot be renamed or deleted, and it keeps its own panel layout.
- Default layout of the Diff tab:
  - **Models** panel, top left;
  - **IfcDiff** panel, bottom left;
  - **Properties** panel, whole right;
  - 3D view in the center.
- The tab is also reachable through **Window > Diff** (BonsaiViewer01) to switch to it from another tab.

### IfcDiff panel

The panel holds, from top to bottom:

1. **Model selectors**: two dropdowns, **Before model** and **After model**, listing every loaded model of the federation (Models panel, BonsaiViewer29). Both are required to run a comparison; a run with one of them empty is refused with a message.
2. **Relationships to compare**: checkboxes for the IfcDiff relationships — `attributes`, `geometry`, `type`, `property`, `container`, `aggregate`, `classification`. Default: `attributes`, `geometry`, `property`.
3. **Options**: `is_shallow` toggle (first difference only vs all differences); **IDS shortlist** file picker (optional `.ids`, BonsaiViewer24) to restrict the property comparison.
4. **Run** button, with a progress bar in the status bar and a summary (elements added / deleted / changed).
5. **Result actions**: the colour legend, the transparency toggle, the property filters and the report button (below).

### Run

- **Run** calls IfcDiff on the two selected models, on the IFC files the viewer already has open.
- The result is stored in the panel: `added` (GlobalIds), `deleted` (GlobalIds), `changed` (GlobalId → list of change kinds), plus the detailed property diff per element.
- Running again replaces the previous result and clears the previous diff colours first (with an undo point, see Undo).
- An **Export JSON** button writes the raw IfcDiff result (`added` / `deleted` / `changed`) to a `.json` file, same format as the IfcDiff CLI.

### Geometry and structure colours (viewport)

The diff colours are a simple colour override, applied like Paint everywhere else in the viewer (BonsaiViewer02): they replace the element's current colour, and are removed by **Clear Diff**, by **Restore the View**, or by a new run.

| Case | Before model | After model |
|---|---|---|
| Unchanged | original colour | original colour |
| Deleted (in Before only) | **red** `#E6194B` | — |
| Added (in After only) | — | **green** `#3CB44B` |
| Modified | **blue** `#3B6FE0` | **orange** `#F08A24` |

- A modified element is coloured in **both** models (blue in Before, orange in After), so the user sees the two states superposed.
- The diff colours are removed by **Clear Diff**, by **Restore the View** (BonsaiViewer02), or by a new run.

#### Transparency toggle

- A button **Swap transparency** switches which modified model is shown at 50 % opacity.
- Default: modified **After** objects at 50 % transparency, modified Before objects opaque — the After state is "on top".
- One click: Before modified objects at 50 %, After opaque. The button state shows which side is transparent.

### Property comparison

#### IDS shortlist

- **Without IDS**: every property set and quantity set of the compared elements is compared (IfcDiff `property` relationship, on all the elements common to both models).
- **With IDS** (`.ids` file loaded in the panel): only the properties named in the requirements of the IDS specifications are compared (BonsaiViewer24 engine). The IDS applicability also restricts **which elements** are compared for properties.

#### Property filter controls

Below the model selectors, a **property field** (BonsaiViewer26) lets the user pick one property (Pset + Property) from the IDS shortlist (or from all the properties when no IDS is loaded) to focus the visualisation on it.

A row of **selection buttons** filters the highlighted / selectable elements by change kind:

- Select elements with a **property added**
- Select elements with a **property removed**
- Select elements with a **value added**
- Select elements with a **value removed**
- Select elements with a **value changed**
- Select elements with a **value increased** (numeric)
- Select elements with a **value decreased** (numeric)

- **Select** puts the matching elements of **both** models in the viewer selection; **Isolate** isolates them. Selection follows the existing selection pipeline (click in the viewport feeds the Properties panel).
- The property field scopes the buttons to the chosen property; empty = all compared properties.

#### Report table for multiple Element

- A button **Show report** opens a dedicated window with a table (same widget as BonsaiViewer07): one row per changed element — Model (Before/After), GlobalId, Class, Name, change kinds, and for properties: Pset, Property, old value, new value, change kind.
- Click on a row selects the element; double-click isolates it; CSV export like the Table panel.



#### Report Table for single Element

On pourrait égalementa voir une second vue.
En colonne, Objet Before , Objet After
En ligne, Model_Name, GUID, Class puis la liste des prorpriétés avec les valeurs.
De cette manière, pour un objet on pourrait facilement voir ce qui a changé.

#### Colours in the Properties panel

When an element of a diffed model is shown in the Properties panel (BonsaiViewer14), its property rows are coloured by the last diff run:

- **green** — property or value added;
- **red** — property or value removed;
- **orange** — value changed (with a tooltip showing old → new).

The colours disappear on **Clear Diff** or a new run, like the IDS Color Properties toggle (BonsaiViewer24 / BonsaiViewer14).

### Undo and state

- Each **Run** stores an undo state before applying the diff colours and transparency (same mechanism as Paint / Hide, BonsaiViewer02): **CTRL+Z** restores the previous view and colours. Depth: 10 states, like the other display actions.
- **Clear Diff** also stores an undo state, so CTRL+Z can bring the diff colours back.
- The panel state (selected models, relationships, IDS path) is kept for the session; the diff colours and report are cleared when a model is loaded or removed.

## Open Subject

- Geometry comparison precision: use the model's `IfcGeometricRepresentationContext.Precision` (IfcDiff default) or a fixed epsilon?
- Should attribute-only changes (e.g. PredefinedType) count as "modified" and be coloured, or only geometry changes?
- Report window: dockable panel or modal window?
- Export: raw IfcDiff JSON only, or also a human-readable CSV/HTML report?
