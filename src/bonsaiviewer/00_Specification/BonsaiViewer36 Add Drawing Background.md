<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer36 - Add Drawing Background

## Introduction

The viewer can import **2D plans** as documents of the **External Documents** tree (BonsaiViewer35). A plan is displayed as a **background of the 3D view**: it is placed flat in the scene at a chosen elevation, with its own rotation, scale and translation, so a 2D drawing can be overlaid on the model and compared with it.

**Phase 1 supports PDF files only.** Raster images (`.png`, `.jpg`, `.svg`) and CAD drawings (`.dxf`, `.dwg`) come in phase 2.

The plan configuration (source file, transform, visibility) is **saved with the project** and can be exported to / imported from a **JSON** file.

This spec covers background plans only. Drawings contained in the IFC itself (IfcAnnotation) are covered by BonsaiViewer19, and the dedicated interactive 2D view by BonsaiViewer40.

## Specification

### 2D Plan documents (External Documents tree, BonsaiViewer35)

- Importing a **`.pdf`** file (phase 1) creates a **2D Plan** document in the External Documents tree (BonsaiViewer35), named after its file and marked with a plan icon or tag, so plans and tables are distinguishable in the tree. Phase 2 adds `.png`, `.jpg`, `.svg`, `.dxf` and `.dwg` with the same document kind.
- **Left-click** on a plan in the tree opens its menu: **Modify Plan**, **Show / Hide**, **Delete**.
- **Right-click** on a plan opens: **Show / Hide**, **Edit Plan** (same window as Modify Plan), **Refresh**, **Locate File…**, **Export Config (JSON)**, **Import Config (JSON)**, **Delete**.
- **Right-click** on the root or a folder of the tree offers **Save Documents (JSON)** and **Load Documents (JSON)** — the "save / load configurations" buttons of the original idea, applied to **all the documents of the tree at once**, plans and tables (BonsaiViewer35; see Persistence).
- Each plan document shows its state in the tree: **shown**, **hidden**, or **file not found**.

### Display in the 3D view

- A **shown** plan is rendered in the 3D view as a flat background at its elevation. In phase 1, the **PDF page** is drawn on a horizontal plane at height Z; phase 2 adds raster images (drawn the same way) and DXF / DWG vector lines (projected on the same plane).
- Several plans can be shown at once; each sits at its own elevation, stacked in the same view as the model.
- **Show / Hide** toggles the visibility of the plan in the view without deleting the document.
- Show / Hide and every transform change store an **undo state**, like Paint / Hide (BonsaiViewer02): CTRL+Z restores the previous visibility and transform (depth 10, like the other display actions).
- A plan is a background: it does not participate in filters, clash or property display. It is **selectable in the 3D view**: clicking on a plan selects it (highlighted) and shows a gizmo to **move** it (translation in its plane) and **rotate** it (around Z); the drag updates the translation / rotation of the configuration and stores an undo state. The Plan Configuration window stays the way to enter exact numeric values. Clicking outside a plan selects the model elements as usual.
- **Phase 2 — DXF / DWG units** — the DXF file is read with a dedicated library (libdxfrw); the units are read from the file header, meters by default.
- No automatic offset against **z-fighting**: when a plan sits exactly at the elevation of a slab, the user adjusts its height Z in the Plan Configuration window.
- Background plans are displayed in the **3D view only**; the dedicated 2D view (BonsaiViewer40) does not show them.

### Plan Configuration window

**Modify Plan** / **Edit Plan** opens a modal window with:

- **Source** — the file path, with a **Change Source…** button (file dialog; the current transform is kept when the source changes).
- **Height (Z)** — the elevation of the plan on the model's Z axis, in meters.
- **Rotation** — the rotation of the plan around X, Y and Z, in degrees.
- **Scale** — a manual multiplication factor applied to the plan's native size (for example `0.001` for a plan drawn in millimeters); no automatic derivation from the paper size or the print scale.
- **Translation** — a 3D offset, expressed either:
  - **relative to the model axes**, or
  - **relative to the plan axes** (following the plan's X / Y / Z rotations).
- **Page** (PDF only) — the page to display; default: the first page.
- **Opacity** — a slider from 0 to 100 % (default 100 %, opaque), so the model stays visible through the plan.
- **Apply** validates and closes: the view updates and an undo state is stored. **Cancel** discards the changes.

### Persistence and JSON configuration

- The tree entry and the whole **plan configuration** — source path, height, rotations, scale, translation, page, opacity, visibility — are **saved with the project**, like every external document (BonsaiViewer35, Refresh and persistence).
- A `.json` configuration file can hold **one or several plan configurations**.
- **Export Config (JSON)** (on a plan) writes the configuration of that plan to a `.json` file: source file name (path relative to the JSON file when possible), height, rotations, scale, translation, page, opacity, visibility.
- **Save Documents (JSON)** (on the root or a folder) writes **all the configurations of the tree** to one `.json` file: every plan configuration (this spec) and every table join configuration (BonsaiViewer35).
- **Import Config (JSON)** (on a plan) and **Load Documents (JSON)** (on the root or a folder) read a `.json` file and apply every configuration it holds: each document of the file is updated in the tree, or created when it is not in the tree yet (plans here, tables in BonsaiViewer35). The source paths are resolved relative to the JSON file; if a source file is not found, its document shows **file not found** with **Locate File…**.
- On project open, each plan is loaded from its stored path if the file exists; otherwise it shows **file not found**.
- **Refresh** re-reads the source file from disk (for example after the plan was edited in another software) and keeps the transform.

### Phase 2 — other formats

- Raster images (`.png`, `.jpg`, `.svg`) — drawn on the plane like a PDF page; no page selector.
- CAD drawings (`.dxf`, `.dwg`) — vector lines projected on the plane; units read from the DXF header (meters by default); the DWG path is still open (see Open Subject).

### Phase 3 — registration (calage)

Not in the first version:

- **Point registration** — the user clicks one point on the plan, then one point on the model; the plan is translated so that the two points coincide.
- **Curve registration** — the user draws a segment on the plan, then selects a segment in the model; the plan is translated, rotated and scaled so that the two segments overlap.

## Implementation notes (status)

Implemented in `modules/external_documents/` (branch `bonsaiviewer35-36-external-documents`):

- Importing a `.pdf` (or a phase-2 image/CAD extension) creates a 2D Plan document in the External Documents tree (BonsaiViewer35), named after its file, with a plan icon; state shown / hidden / file not found.
- Left-click on a plan opens its menu (Modify Plan, Show / Hide, Delete). Right-click opens Show / Hide, Edit Plan, Refresh, Locate File…, Export Config (JSON), Import Config (JSON), Delete. Right-click on the root or a folder offers Save / Load Documents (JSON) for all the documents of the tree (plans and tables).
- Display in the 3D view (`View.cpp::updatePlanOverlays` + `PlanOverlay.cpp` + `PdfRender.cpp`): a shown plan is drawn as a flat quad at its height Z — the **rasterised PDF page** textured on the quad through the image-quad overlay channel (`ViewportWindow::setOverlayImages` → `OverlayRenderer::encodeOverlayImages`, drawn in the main MSAA pass so it is depth-tested against the model, alpha = the plan's opacity), plus a per-plan outline (`setOverlayLines`) and a name label (`setOverlayLabels`). Several plans can be shown at once. The plan's native size comes from the PDF page MediaBox (points → meters), multiplied by the manual scale; 1 pt = 25.4/72000 m. The page is rasterised by Poppler through its CLI (`pdftoppm`, else `pdftocairo`) at ~2048 px/m capped at 4096 px on the long side, cached by path + page + size + mtime (`View::planPageImage`); when no rasterizer is available the quad falls back to a translucent fill through the highlight-triangle channel (`setHighlightTriangles`). No z-fighting offset: the user adjusts Z in the Plan Configuration window.
- Plan Configuration window (`Dialogs.cpp` `PlanDialog`): source with Change Source… (transform kept), Height Z, Rotation X/Y/Z, Scale, Translation X/Y/Z with a relative-to-plan-axes option, Page (PDF only), Opacity slider 0–100 %. Apply validates and closes; Cancel discards.
- Show / Hide and every transform change store an undo state (`UndoStack::record` before the change, depth 10 like the other display actions).
- Persistence: the plan configuration (source, height, rotations, scale, translation, page, opacity, visibility) is saved with the project in the `.ifcfed` extension `bonsaiviewer_external_documents`. Export / Import Config (JSON) on a plan writes / reads one plan's configuration (source path relative to the JSON file when possible); Save / Load Documents (JSON) on the root or a folder handles all the documents (BonsaiViewer35). On project open, each plan loads from its stored path if the file exists, else shows file not found; Refresh re-reads the source and keeps the transform.
- 3D view only: the dedicated 2D view (BonsaiViewer40) does not show the background plans.

Phase 1 limits (open, see Open Subject):

- PDF rasterization goes through the Poppler CLI tools (no link-time dependency); linking a PDF library directly (Poppler/MuPDF/Qt Pdf) is still the open subject below. Where the CLI is absent, the plan shows as a translucent fill + outline + label.
- The fallback filled quad shares the highlight-triangle channel with the area measurement tool; the textured page, the outline and the label stay on their own channels, and the fill is re-asserted on the next plan update.
- Plans are not selectable in the 3D view yet (no gizmo); the Plan Configuration window is the way to move / rotate them numerically. Clicking in the view selects model elements as usual.

## Open Subject

- ~~PDF rendering library for phase 1~~ — resolved for phase 1: Poppler via its CLI (`pdftoppm`/`pdftocairo`), ~2048 px/m capped at 4096 px on the long side. Linking Poppler/MuPDF/Qt Pdf directly (in-process, no subprocess) remains open for a later phase.
- Exact JSON schema: field names, version field, and how a loaded JSON is merged with the existing tree (replace every plan, or update only the plans the file holds?).
- DWG support (phase 2): libdxfrw reads DXF only — which path for DWG (ODA File Converter, Teigha, or a conversion to DXF first)?
- Gizmo in the 3D view: move and rotate around Z only (current choice), or full X / Y / Z rotation with the gizmo?
