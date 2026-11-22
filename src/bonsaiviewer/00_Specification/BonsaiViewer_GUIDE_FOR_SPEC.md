# BonsaiViewer — Guide for Writing Specifications (prompt guide)

**Purpose.** Paste this guide into your prompts when asking to write a new
BonsaiViewer spec, so that every spec is precise and covers all the necessary
subjects. **All specs are written in English** (UI labels and IFC terms stay
as-is).

---

## 1. File conventions

- Location: `src/bonsaiviewer/00_Specification/`
- Name: `BonsaiViewerNN <Title>.md`, with NN the next free number.
- Top of file: `# BonsaiViewerNN - <Title>`, then a status banner if the spec
  is not final: `Status: idea` / `Status: draft, to validate`.
- **Every spec — even one that starts as an idea — targets the full canvas
  below**; the status banner only records progress.
- Mandatory skeleton:
  - `## Introduction` — what the feature is and why; cross-references to
    related specs (`BonsaiViewerNN`).
  - `## Specification` — the body.
  - `## Open Subject` — open questions; keep the section even when empty.
- Optional sections when relevant: `## Decisions` (with
  `Decided (YYYY-MM-DD): ...`), delivery phases (`## Specification Phase 1 /
  2`), `## Implementation`, `## Performance`, `## Example ...`.

## 2. Mandatory subjects (checklist — every spec answers each applicable item)

### 2.1 Tab organization — only if the feature creates a new ribbon tab
If yes, describe: the default layout of the tab (which panels, where docked,
central 3D view), how tabs are added / renamed / deleted / reordered, and how
layouts are saved and restored at start-up (cross-ref BonsaiViewer21).
If no new tab: just state which existing panel the feature lives in and where
it is docked.

### 2.2 Panels
For each panel: name, where it docks (left / right / bottom, tabbed with which
panels), how it is opened (ribbon button, Window menu, shortcut), whether it
is visible at start-up, and the **empty state** (no model loaded / empty
panel).

### 2.3 Panel buttons
For each toolbar button / control: **label + what it does + when it is
disabled** (e.g. "Run is disabled without a selected filter").

### 2.4 Right-click menus
For **every clickable object type** in the panel — including a right-click on
empty space — **list the functions of the context menu** (as a list of
functions; a table keyed by object type is welcome but not required — model:
BonsaiViewer06 Filter Rule).

### 2.5 Edit views
If an object can be modified, describe the **full edit window**: every field,
the validation rules, and the OK / Cancel behavior. If the edit window is
already specified elsewhere, cross-reference it (`BonsaiViewerNN`).

### 2.6 Save / load configuration
As soon as the panel configuration is complex (any named list: filters,
tables, rules, columns, layouts...), state **where it is persisted** (project
file / application-cache JSON / export–import file) and **whether it is
reloaded at start-up**. If nothing is saved, say so explicitly ("session only,
nothing is saved").

### 2.7 Click behaviors — always write all three
For every panel, explicitly describe, for each object type: **single
left-click, double left-click, single right-click**. Do not rely on a default
convention; write all three every time (model: "Table interactions" in
BonsaiViewer07 Table).

### 2.8 Right-click functions per object type
Checklist counterpart of 2.4: row, node, folder, header, column, empty
space... each specific clickable object has its right-click functions listed.

## 3. Transversal behaviors — mention each time one applies

- **Undo**: does the action create a CTRL+Z point? (cross-ref BonsaiViewer02 / 27)
- **Selection sync**: clicks in the panel select in the 3D view, and vice versa.
- **Status bar**: result message, error message, progress bar during long runs.
- **Empty state**: what the panel shows when empty or when no model is loaded.
- **Keyboard shortcut**: list it, cross-ref BonsaiViewer31 Keybindings.
- **Drag & drop**: what can be dragged and the rules (e.g. a group cannot be
  dropped into itself or a descendant).

## 4. Scope rule

Default scope of every function = **all loaded models**. State explicitly when
a function is limited to the active model or to the current box.

## 5. Conventions

- Cross-references between specs: `BonsaiViewerNN`.
- Decisions: `Decided (YYYY-MM-DD): ...`
- Delivered in phases: mark each phase (done / to build and test).
- Examples shipped at first start: describe the Examples folder behaviour and
  its version setting.
- Performance: if the feature is heavy, add a `## Performance` section
  (diagnosis + steps).

## 6. Reference examples (templates)

- Right-click menus per object type: **BonsaiViewer06 Filter Rule**.
- Click behaviors (single / double / right click): **BonsaiViewer07 Table**,
  section "Table interactions".
- Full panel spec (panel + buttons + menus + edit window + save/load + clicks):
  **BonsaiViewer35 ExternalTablel**, **BonsaiViewer10 Manage BCF**,
  **BonsaiViewer24 IDS**.
- Tab organization + layout save/load: **BonsaiViewer21 Add Panels**.

## 7. Prompt to use when writing a new spec

```
Write the specification BonsaiViewerNN - <Title> for BonsaiViewer, in
English, following this guide. Cover every applicable mandatory subject:
tab organization (only if a new tab is created), panels, panel buttons,
right-click menus per clickable object, edit views, save/load of the
configuration, click behaviors (single click / double click / right click,
written explicitly for each object type), scope (default: all loaded models),
and the transversal behaviors (undo, selection sync, status bar, empty state,
shortcut, drag & drop). Use the skeleton Introduction / Specification /
Open Subject, cross-reference related specs with BonsaiViewerNN, and add a
status banner if the spec is an idea or a draft.
```

## 8. Review checklist before finishing a spec

- [ ] English; file named `BonsaiViewerNN <Title>.md`; status banner if not final
- [ ] Introduction + Specification + Open Subject
- [ ] New tab? → tab organization described; otherwise panel + dock position stated
- [ ] Every panel: how it is opened, empty state
- [ ] Every button: label + action + disabled condition
- [ ] Every clickable object: right-click functions listed (including empty space)
- [ ] Editable objects: full edit window described or cross-referenced
- [ ] Complex configuration: where saved / loaded, or "nothing is saved" stated
- [ ] Single click / double click / right click written explicitly for each object type
- [ ] Scope stated (default: all loaded models)
- [ ] Transversals: undo, selection sync, status bar, shortcut, drag & drop — each when applicable
