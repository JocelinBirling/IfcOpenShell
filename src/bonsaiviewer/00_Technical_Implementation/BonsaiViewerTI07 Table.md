<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI07 - Table

Spec source: `../00_Specification/BonsaiViewer07 Table.md`,
`../00_Specification/BonsaiViewer26 Property Selection.md`

## What exists today (phase 1)

- `services/PropertyCatalog` (B05, spec 26): property sets and quantity
  sets of all the loaded models (`IfcDataAccess::propertySets` /
  `quantitySets` on every element of the registry), with element counts
  and up to three sample values. Scanned 250 elements per event-loop turn
  on the GUI thread (express access stays single-threaded); `ensureBuilt()`
  starts a scan when stale; `modelsChanged` / `modelDataSourceReady` mark
  it stale (and restart a scan in progress); `updated()` when complete.
- `modules/property_selection/` (spec 26):
  - PropertyField: Pset and Property QLineEdits with QCompleters
    (contains, case ignored). The completion role holds the name to insert,
    the display role the label ("FireRating — Pset_WallCommon — REI60");
    choosing a property fills its Pset; a filled Pset restricts the property
    model to that set.
  - PropertyPicker: moved from `modules/filters/`, fed by the catalog maps.
- `modules/filters/SelectionEditor`: the IDS specification editor, shared
  with the filter layers (TI06).
- `modules/table/`:
  - Table.h: `TableColumn` (kind Property / Name / Class / GlobalId /
    Model, title, pset, property, auto_numeric) and `TableDefinition`
    (name, description, `SelectionFields`, columns).
  - TableModel: QAbstractTableModel; rows = object ids; a row's values are
    read through B05 the first time the row is displayed, then cached.
    Values are double / qlonglong / QString; AutoConvertTextToNumeric
    turns "12,5" or "12.5" into a double (a text with both separators stays
    a text). Numbers are right-aligned.
  - TableCreationDialog: name, description, SelectionEditor (the preview
    works on the active model).
  - ColumnDialog: kind, PropertyField, column name, AutoConvertTextToNumeric.
  - Panel: docked at the bottom (replaces the "Spreadsheet"
    placeholder; the ribbon button is "Table", the panel key stays
    `sheets`).
  - View (TablePanelView): evaluation = one `IdsBridge::preview` per loaded
    model (IFC path from the loader), in sequence, EXPRESS ids mapped to
    object ids through the element registry; a generation counter drops
    stale replies. Header click = column menu; header drag = the header
    move is undone and the column moved in the definition (visual order =
    column order). Row selection -> B01 `setSelection(Table)`; double click
    -> B08 Isolate. CSV export through QSaveFile, separator and decimal
    point from the system locale, UTF-8 with BOM.
- MainWindow creates the catalog and the table view after the filters.

## Phase 2, part A (implemented, to build and test)

- `modules/table/TableStore` (B14 PersistenceService, document
  `tables.json`): folders and tables (`TableDocument` = id, folder,
  `TableDefinition`). The IDS selection is saved with `IdsFacet::toJson`;
  columns with their kind, title, pset, property and auto_numeric. Import /
  export of one table (format key `columns`, version 1).
  Same model as `services/FilterStore` (TI06).
- `modules/table/TablesPanel`: the Tables panel (bottom dock, tabbed with the
  Table panel; key `tables`). Toolbar: Table (create, at the selected folder
  or at the root), Folder (optional), Run, Import. The Table panel has no
  creation action any more: it only modifies, saves, and evaluates the
  loaded table.
- `modules/table/TablesView`: B12 tree data source. Context menus, rename,
  delete (with confirmation), drag and drop, double click = load. Emits
  `openRequested(id)` and `newTableRequested(folder)`.
- `modules/table/View` (TablePanelView): `saveTable()` (add or update),
  `openTable(id)` (load, then evaluate), `newTable(folder)`. The table name
  shows “(not saved)” until saved. A table deleted in the tree becomes
  unsaved. Save button in the Table panel toolbar.
- `MainWindow`: creates `table_store_` and `tables_view_`, wires the two
  signals, adopts the Tables panel in PanelManager, adds the toggle to the
  QUERY ribbon group.

Not done yet: apply table to box, Count and Color columns, grouping,
relations, calculated columns (spec 23).

## Building blocks used

- B01 SelectionService — row click.
- B05 IfcDataAccess — cell values, property catalog.
- B06 SelectionEditor / IdsBridge (TI06) — the table selection.
- B08 ActionPipeline — double click (isolate), "Isolate in the view".
- B17 PopupService — validation and evaluation errors.

## Technical questions

- The worker keeps one IFC in memory: a table over several models reopens
  each file at every evaluation. A per-path cache in the worker would make
  Refresh cheaper on federations.
- Large tables: the cells are lazy, but the CSV export reads every row.
