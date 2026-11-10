<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer35 - External Table

## Introduction

The viewer can import an **external table** — a spreadsheet file (`.xlsx`) or a CSV file — that holds **one BIM object per row**, outside of the IFC models. The table is **joined to the loaded models** through a key: one column of the file matched against one identifier of the objects (GlobalId, Name, or any property). Once imported, the table can be browsed, sorted, filtered and exported in parallel with the 3D model, and its rows drive the viewer selection.

The import is **temporary in content, persistent in configuration**: the data itself is not embedded in the project. The external documents are listed in their own tree; the file path and the join settings of each document are saved with the project, and the data can be re-visualized and refreshed at any time.

## Specification

### External Documents tree

- A new **External Documents** panel lists the imported external files in a **tree**, with optional folders — same organisation as the Tables panel (BonsaiViewer07, List Of Table): create folder, import document, rename, delete, move by drag and drop.
- The tree holds **two kinds of documents**, detected from the file extension:
  - **External Table** (`.xlsx`, `.csv`) — joined to the models and shown in a grid, specified in this document;
  - **2D Plan** (`.pdf` in phase 1, then `.png`, `.jpg`, `.svg`, `.dxf`, `.dwg`) — shown as a background of the 3D view, specified in BonsaiViewer36.
- Default position: bottom left of the **Table** tab (BonsaiViewer21), next to the Tables panel. The panel can also be added to any user tab from the panel catalogue.
- Right-click on a document opens the actions of its kind — table: **Visualize** (open its grid), **Refresh**, **Edit Join**, **Locate File…**, **Delete**; plan: **Show / Hide**, **Edit Plan**, **Refresh**, **Locate File…**, **Export / Import Config (JSON)**, **Delete** (BonsaiViewer36). Right-click on a folder or the root: **Add Folder**, **Import Document**, **Rename**, **Delete**.
- Each document shows its state: **loaded**, **file not found**, or — for a table — **key column missing**.

### Import

- **Import Document** opens a file dialog accepting the table files (`.xlsx`, `.csv`) and the plan files — `.pdf` in phase 1, then `.png`, `.jpg`, `.svg`, `.dxf`, `.dwg` in the later phases (BonsaiViewer36). Several files can be selected at once; each becomes one document of the matching kind in the tree, named after its file.
- **Tables — CSV parsing** follows the CSV export rule of BonsaiViewer07 in reverse: the file is UTF-8; the field separator is `;` when the system decimal separator is `,`, otherwise `,`; the first row gives the column names.
- **Tables — one object per row**: the file is expected to hold one object per row. **Duplicate key values**: only the first matching row is kept — the table has one row per object; the duplicate rows are listed in the Match report.

### Join configuration window

Each document has a join configuration, edited in a dedicated window:

- **Model key** — the identifier of the objects used for the join: **GlobalId**, **Name**, **Class**, or a **Property** chosen with the standard property field (BonsaiViewer26: Pset and Property inputs with completion on all the loaded models, and Browse…). Default: GlobalId.
- **Table key column** — the column of the imported file that holds the key, picked in a dropdown listing the file's columns. Default: the first column.
- **Sheet** (`.xlsx` only) — a dropdown lists the sheets of the file; default: the first sheet. Changing the sheet reloads the column list and the preview.
- **Get Lucky** — tries to detect the join automatically: it reads the **first 10 rows** of the file and compares each column's values with the loaded models — **GlobalIds, Names, and the properties of the property catalog** (BonsaiViewer26). It proposes the candidate (key column, model key) pairs that match, best first: a column matches when a **majority of its 10 values** (for example 8 of 10, tolerating empty cells) match distinct loaded objects. The user picks a proposal or keeps the manual choice.
- The window shows a preview of the first 10 rows of the file, with the chosen key column highlighted.

### External Table panel (grid)

- **Visualize** on a document opens its grid in a dedicated **External Table** panel, docked at the **bottom** of the window (same place as the Table panel of BonsaiViewer07). One document at a time, like one table at a time.
- The grid shows **all the columns of the file**, plus the model columns **Model**, **Class**, **Name**, **GlobalId** of the joined object, at the left.
- Toolbar: **Refresh** (re-read the file), **Edit Join**, **Export CSV**; next to it the document name and the counts (rows, matched rows, matched objects).
- **Sort and filter** on the columns are the ones of the Table panel phase 3 (BonsaiViewer07): an **arrow** and a **funnel** on each column header — sort A→Z, Z→A, then back to the file order; funnel with the distinct values, checkboxes, search field, All / None, Apply. The summary shows "12 of 40 row(s)" while a filter hides rows.
- **CSV export** — writes **all the rows of the file** (not only the loaded ones), in the displayed order when the grid is sorted, else in file order; same format rule as BonsaiViewer07.
- **Large files** — the grid loads at most **10,000 rows**; a banner shows "10,000 of 45,320 row(s) loaded". Sort and filter apply to the loaded rows.
- Columns can be moved by left drag on the header.

### Table interactions

- **Click on a row** — selects the joined object(s) in the viewer (several rows with Ctrl / Shift).
- **Double-click on a row** — isolates the joined object(s). If the clicked row is not selected, only that row is isolated.
- **Right-click on a row** — menu **Isolate**, same rule as the double-click.
- A row whose key matches no loaded object shows empty model columns and is greyed; it selects nothing.

### Match report

After each load, refresh or join change, the panel computes:

- **matched rows** — file rows whose key matches at least one loaded object (with duplicate keys, only the first matching row counts as matched);
- **unmatched rows** — file rows with no matching object;
- **duplicate rows** — file rows whose key was already matched by an earlier row (not shown in the grid, listed here);
- **unmatched objects** — loaded objects whose key value has no row in the file.

The toolbar shows the counts; a **Show Report** button opens a small window listing the unmatched rows, the duplicate rows (the file's columns) and the unmatched objects (Model, Class, Name, GlobalId, key value), with click-to-select on the object rows.

### Refresh and persistence

- **Refresh** re-reads the file from its stored path and re-applies the join; the grid updates. If the file changed on disk (columns added or removed), the join configuration is kept: the key column is matched by name; if it disappeared, the document goes to the **key column missing** state until the join is edited.
- The **tree and the configuration of every document** are **saved with the project**: for a table, its join configuration (file path, sheet, model key, table key column, name, folder); for a plan, its plan configuration (BonsaiViewer36). The file contents are not saved.
- **External Documents tree JSON** — Save / Load on the root or a folder of the tree (BonsaiViewer36) writes / reads **one `.json` file holding the configurations of all the documents of the tree**: the plan configurations and the table join configurations. There is no separate export of a join configuration in v1.
- On project open, each document is loaded automatically if its file exists; otherwise it shows **file not found**, with a **Locate File…** action (file dialog; on success the document loads, and joins for a table).
- When models are loaded or removed, the join is re-evaluated on the next **Visualize** or **Refresh**: the matched / unmatched counts and the greyed rows update.

### Relations to the rest of the viewer

- The external rows drive selection and isolation only; the external values are **not written into the IFC models**.
- **v1 is read-only**: the external values do not feed the filter rules (BonsaiViewer06) and cannot colour the objects; the external grid has no calculated columns (BonsaiViewer23) and no Color column (BonsaiViewer07).
- The external grid is independent from the Tables panel: a saved table (BonsaiViewer07) does not read the external columns.

## Open Subject

None for v1 — every point is decided above. Candidates for a later version: calculated columns (BonsaiViewer23) and a Color column (BonsaiViewer07) in the external grid; external values as filter criteria (BonsaiViewer06) or colour rules.
