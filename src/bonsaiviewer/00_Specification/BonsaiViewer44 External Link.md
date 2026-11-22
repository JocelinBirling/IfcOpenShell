<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer44 - External Link

Status: draft, to validate

## Introduction

From the **Properties panel** (BonsaiViewer14), the user can attach **external links** to an object — a **file**, a **folder**, or a **web page**, optionally with a **locator** (page, section, anchor, paragraph) that announces the relevant excerpt. A link can be **opened** (OS default application / browser) or **previewed** in a small floating window inside BonsaiViewer that shows the announced excerpt — e.g. the paragraph of a document, the page of a PDF. A link to a **folder** shows the list of the documents it contains.

Links are viewer-side metadata keyed by GlobalId: they follow the object across all the loaded models and are saved with the project, but are not written into the IFC in v1 (see Open Subject for an IFC export).

Related specs: BonsaiViewer02 Display Function, BonsaiViewer07 Table, BonsaiViewer14 Properties Panel, BonsaiViewer19 Display Drawings, BonsaiViewer21 Add Panels, BonsaiViewer27 CtrlZ, BonsaiViewer31 Keybindings, BonsaiViewer35 External Table, BonsaiViewer36 Add Drawing Background, BonsaiViewer40 2D View.

## Specification

### Where the feature lives (no new tab)

- No new ribbon tab. The feature is a **Links section of the Properties panel** (BonsaiViewer14), docked whole right in every default tab (BonsaiViewer21), shown for the selected element below the property sets.
- **Empty states**: no object selected → the section shows "Select an object to see its links"; selected object without links → "No link yet — use Add Link or right-click."

### Link model

- A link = **name**, **type** (file / folder / web), **path or URL**, optional **locator** (`page=3` for a PDF, `#anchor` for a web page or HTML file, a section reference such as `2.3`, or a paragraph search text), and a **target**.
- Target = the **object** shown in the Properties panel (matched by **GlobalId**, stable across all the loaded models), or a specific **property** (GlobalId + Pset + Property) when added from a property row.
- Several links per object / per property: allowed, shown as a list.

### Links section of the Properties panel

- Header "**Links**" with the count; each link row shows an icon by type (file, PDF, image, folder, web), the name, and a badge with the locator when set.
- Toolbar button: **Add Link** — opens the Link window targeting the shown object. Always enabled when an object is selected; **disabled when no object is selected**.
- Right-click menus:
  - on a **link row**: **Open**, **Preview**, **Edit Link**, **Copy Path / URL**, **Remove Link**;
  - on the **Links section header**: **Add Link**, **Expand All / Collapse All**, **Export Links (JSON)**, **Import Links (JSON)**;
  - on **empty space** of the section: **Add Link**, **Import Links (JSON)**;
  - on a **property row** (BonsaiViewer14): the existing menu (Copy the value, Isolate similar, Select similar) gains **Add Link to this Property**.
- Click behaviors:
  - **link row** — single left-click: selects the row and selects the linked object in the 3D view (selection sync); double left-click: **Open**; right-click: row menu;
  - **folder link row** — single left-click: select; the expand arrow toggles the contained documents; double left-click: **Open** (the folder in the OS file manager); right-click: row menu;
  - **contained document row** (under an expanded folder link) — single left-click: select; double left-click: **Open**; right-click: **Open**, **Preview**, **Add as Link…**;
  - **property row** — unchanged from BonsaiViewer14; right-click additionally offers **Add Link to this Property**.

### Link window (edit view)

Opened by **Add Link** / **Edit Link** / **Add Link to this Property**. Fields:

- **Name** — default: the file name, folder name, or page title.
- **Type** — File / Folder / Web page, auto-detected from the input, editable dropdown.
- **Path / URL** — text field; **Browse…** opens a file or folder dialog for File / Folder, depending on Type.
- **Locator** — free text: `page=3`, `#anchor`, `section 2.3`, a paragraph text… **Pick in Preview…** opens the Link Preview window to capture the current excerpt into this field.
- Validation: **OK** is disabled while Path / URL is empty; for File / Folder, the path must exist (error message otherwise); a web URL is always accepted. **OK** saves the link (one undo point) and closes; **Cancel** discards.

### Open vs Preview

- **Open** — file or folder: OS default application (`QDesktopServices::openUrl`); web page: default browser.
- **Preview** — a small floating, non-modal **Link Preview window** inside BonsaiViewer, showing the announced excerpt:
  - **PDF** — the page from the locator (`page=N`, default 1), rendered; **Prev / Next page** buttons;
  - **image** (`.png`, `.jpg`, `.svg`) — the image, fit to the window;
  - **text-like** (`.txt`, `.md`, `.csv`, `.ifc`…) — the excerpt: the paragraph containing the locator text (or line N), highlighted;
  - **web page** — the page in an embedded view, scrolled to `#anchor` (fallback: open in the browser);
  - **folder** — the list of the contained documents (name, size, date); the rows behave like the contained document rows above, with **Add as Link…** to link one of them directly.
  - The window has a **Link this excerpt** button — writes the current page / paragraph / anchor back into the link's Locator (an edit, one undo point), so the link announces exactly what was previewed.

### Scope

- Links match their target by **GlobalId across all the loaded models**; every function applies to all the loaded models. A link whose target is not in any loaded model is shown greyed in exports, and re-attaches automatically if the object is loaded later.

### Save / load configuration

- The **links are saved with the project** (like the external documents, BonsaiViewer35): target GlobalId (+ Pset / Property for property links), name, type, path / URL, locator; reloaded at start-up. Paths are stored relative to the project file when possible (BonsaiViewer36).
- **Export Links (JSON)** / **Import Links (JSON)** on the Links section header write / read one `.json` file with the whole list; import merges and skips duplicates (same target + same path / URL).
- Nothing is written into the IFC model in v1.

### Transversal behaviors

- **Undo** (BonsaiViewer02 / BonsaiViewer27): adding, editing (including a locator captured in the preview) or removing a link creates a CTRL+Z point.
- **Selection sync**: selecting a link row selects the linked object in the 3D view; selecting an object in the 3D view shows its links in the Properties panel.
- **Status bar**: "Link opened", errors "File not found", "URL unreachable", progress while a web preview loads.
- **Empty states**: see Panels.
- **Keyboard shortcut**: none in v1 (BonsaiViewer31).
- **Drag & drop**: dropping a file / folder from the OS file manager, or a URL from a browser, onto the Links section opens **Add Link** pre-filled with it.

### Delivery phases

- **Phase 1**: links on objects (file / folder / web + locator), Links section, Link window, Open, Preview for PDF / image / text / folder, save with the project, JSON export / import, undo, drag & drop onto the Links section.
- **Phase 2**: links attached to a single property (**Add Link to this Property**), embedded web preview, expandable folder content directly in the list, IFC export of the links (IfcDocumentInformation / IfcRelAssociatesDocument), drag & drop onto the 3D view to attach to the picked object.

## Performance

- Folder listings are read on demand and cached until the link is refreshed; a **Refresh** action on a folder link re-reads it.
- Web previews load asynchronously with a progress indicator in the status bar; a failed load shows the error and falls back to **Open** in the browser.

## Decisions

- Decided (2026-11-22): the feature lives in the Properties panel (BonsaiViewer14) as a Links section — no new tab.
- Decided (2026-11-22): links are viewer-side metadata saved with the project, keyed by GlobalId; not written into the IFC in v1.
- Decided (2026-11-22): **Open** uses the OS default application / browser; **Preview** is a small floating window in BonsaiViewer showing the located excerpt, with a **Link this excerpt** capture button.

## Open Subject
- IFC-native export of the links (IfcDocumentInformation + IfcRelAssociatesDocument, locator in the document reference) — phase 2, to exchange links with other tools.
- PDF rendering library for the preview — same open question as BonsaiViewer36.
- Web preview engine (embedded web view) and offline behavior.
- Drag & drop onto the 3D view: attach the dropped link to the object under the cursor.
- Locator syntax standardization across types (a small grammar: `page=`, `#`, `section`, free text).
