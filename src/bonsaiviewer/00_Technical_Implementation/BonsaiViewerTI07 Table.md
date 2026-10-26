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
  - Table.h: `TableColumn` and `TableDefinition` (name, description,
    `SelectionFields`, columns).
  - TableCreationDialog: name, description, SelectionEditor (the preview
    works on the active model).
  - ColumnDialog: kind, PropertyField, column name, AutoConvertTextToNumeric.
  - Panel: docked at the bottom (the ribbon button is "Table", the panel key
    stays `sheets`).
  - View (TablePanelView): evaluation = one `IdsBridge::preview` per loaded
    model (IFC path from the loader), in sequence, EXPRESS ids mapped to
    object ids through the element registry; a generation counter drops
    stale replies. Header right click = column menu (`QHeaderView` custom
    context menu, `logicalIndexAt`); header drag = the header
    move is undone and the column moved in the definition (visual order =
    column order). CSV export through QSaveFile, separator and decimal point
    from the system locale, UTF-8 with BOM.

## Phase 2, part A (implemented, to build and test)

- `modules/table/TableStore` (B14 PersistenceService, document
  `tables.json`): folders and tables (`TableDocument` = id, folder,
  `TableDefinition`). The IDS selection is saved with `IdsFacet::toJson`.
  One file for all the tables. Same model as `services/FilterStore` (TI06).
  Import / export of one table (format key `columns`, version 1).
- `modules/table/TablesPanel`: the Tables panel (bottom dock, tabbed with the
  Table panel; key `tables`). Toolbar: Table (create, at the selected folder
  or at the root), Folder (optional), Run, Run on Box, Import.
- `modules/table/TablesView`: B12 tree data source. Context menus (Run, Run on
  the Box, Export, Delete), rename, delete (with confirmation), drag and drop,
  double click = Run. Emits `openRequested(table_id, on_box)` and
  `newTableRequested(folder)`.
- `modules/table/View` (TablePanelView): `saveTable()` (add or update),
  `openTable(id, on_box)` (load, then evaluate), `newTable(folder)` (created
  from the Tables panel, saved at once). The table name shows "(not saved)"
  until saved. A table deleted in the tree becomes unsaved. The Table panel
  has no creation action: it modifies, saves, edits columns and evaluates.
- `MainWindow`: creates `table_store_` and `tables_view_`, wires the signals
  (`openRequested` → `TablePanelView::openTable`), adopts the Tables panel,
  adds the toggle to the QUERY ribbon group.

## Phase 2, part B (implemented, to build and test)

- `Table.h`: `TableColumn::Kind` gets `Count` and `Color`.
- `TableModel`: `Count` is the number of elements of the row; `Color` is an
  empty value, the cell background comes from `row_colours_`
  (`setRowColours`, role `BackgroundRole`).
- Run on Box (not a table property, nothing saved):
  - The Box toggle of the Table panel is removed.
  - `TablesPanel` has "Run on Box" (toolbar and tree menu "Run on the Box");
    it calls `openTable(id, true)`. Plain Run calls `openTable(id, false)`.
  - `View`: `apply_to_box_` (runtime) keeps only the ids present in
    `EntitySetStore::current()` (B07) in `finishEvaluation`. While it is set,
    `EntitySetStore::currentChanged` re-evaluates. The summary shows
    "(on the box)".
- Colour: `applyRowColours` clears `AppearanceService::Layer::TableColour`,
  then for a `Color` column draws one `QColor` per row (`randomColour`, hsv)
  and applies it to all the elements of the row with
  `setLayerColour(objects(row), colour, TableColour)` (B04).
  Called by `applyColumns` and `finishEvaluation`.
- Isolation: the double click and the right-click menu "Isolate" act on the
  selected rows, or on the clicked row when it is not selected
  (`rowsForAction`), through B08 `Isolate`. Selection (B01) follows the rows.
- `ColumnDialog`: Count and Color in the Data list.

Known limit: `applyRowColours` calls `setLayerColour` once per row, so each
call refreshes the viewport. Large tables may be slow; a batched call in
AppearanceService would fix it.

## Phase 2, part C (implemented, to build and test)

- `Table.h`: `TableColumn::group_key` (bool, default false) and
  `TableColumn::aggregation` (`Aggregation`: Default, Concatenate, Sum,
  Average, Min, Max, Count).
- `TableModel`: a row is a group of elements.
  - `regroup()` builds `groups_`, one vector of object ids per row. With no
    group-key column, one element per row. With group-key columns, the
    elements sharing the same key values form one row, in first-seen order
    (a `QHash` on the joined key values, read through `elementValues`).
  - `objects(row)` returns the ids of a row; `setRows` and `setColumns`
    regroup.
  - `rowValues(row)` reads every element of the group, then `aggregate()`
    per column:
    - group-key column: the shared value (the first one);
    - Count column: the group size; Color column: empty (the background);
    - Default: numbers are summed; texts are joined with "; "; a mix gives
      `sum; text; text` (validated);
    - Concatenate: the non-empty values as text, joined with "; ";
    - Count: the number of non-empty values;
    - Sum, Average, Min, Max: numbers only; a text value gives "ERROR".
    - Empty values are ignored; no value gives an empty cell.
- `TableStore`: each column saves `group_key` and `aggregation` (`default`,
  `concatenate`, `sum`, `average`, `min`, `max`, `count`). A missing key reads
  as the default, so existing files load unchanged.
- `ColumnDialog`: Aggregation combo (disabled for Count and Color). The group
  key is not set here; it is kept from the column.
- `CreationDialog`: "Grouping" box, a checkable list of the columns (Count and
  Color excluded). Checked columns become group keys on accept.
- `View`: selection, isolation and colours act on all the elements of the
  rows (`objects(row)`), not on one id per row.

Not done yet: relations, calculated columns (spec 23).

## Building blocks used

- B01 SelectionService — row click.
- B04 AppearanceService — Color layer.
- B05 IfcDataAccess — cell values, property catalog.
- B06 SelectionEditor / IdsBridge (TI06) — the table selection.
- B07 EntitySetStore — the box (Run on Box).
- B08 ActionPipeline — double click and "Isolate" (isolate the rows).
- B12 TreePanelFramework — the Tables tree.
- B14 PersistenceService — `tables.json`.
- B17 PopupService — validation and evaluation errors.

## Technical questions

- The worker keeps one IFC in memory: a table over several models reopens
  each file at every evaluation. A per-path cache in the worker would make
  Refresh cheaper on federations.
- Large tables: the cells are lazy, but the CSV export reads every row.
- Grouping reads the key values of every element when the rows are built
  (`regroup`), so a grouped table over a big model costs one property read per
  element at each evaluation and each column change.
