<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI14 - Select By Property

Spec source: `../00_Specification/BonsaiViewer14 Select By Property.md`

## What exists

`modules/properties` shows the properties of the selected object as key-value
tables. The rows are `components::KeyValueTable` rows.

## Building blocks used

- B05 IfcDataAccess — `propertySets` / `quantitySets` per element.
- B01 SelectionService — « Select similar » (`SelectionSource::Service`).
- B08 ActionPipeline — `run(Isolate)`, `run(RemoveFromView)`, `colour`.
- B15 ContextMenuBuilder — the two context menus.
- Table panel (TI07) — `TablePanelView::showTemporaryTable` for « Colour by property ».

## Changes

- `components/KeyValueTable`: signal `contextMenuRequested(row, on_value, global_pos)`
  on the key and value labels (custom context menu policy).
- `modules/properties/Types.h`: `PropertyContextHandler`, called with the
  property set (as shown, under the filter), the row, `on_value` and the position.
- `modules/properties/Panel`: signals `propertyNameContextRequested(pset, property, pos)`
  and `propertyValueContextRequested(pset, property, value, pos)`; the sets of the
  Properties and Quantities sections forward their row clicks through the handler.
- `modules/properties/View`: owns the two menus and the colouring. Constructed in
  `MainWindow::setupLoader`, after the services it uses; `setTableView` gives it the
  Table panel view.
- `modules/table/View`: `showTemporaryTable(name, columns, ids)` sets the rows
  directly (`temporary_` flag). While the flag is set, `evaluate` and `saveTable`
  do nothing; `Modify` regroups the rows only.

## Key flows

`scan(pset, property, box_only)`: the scope is every element of the loaded
models (basic info of each model in the element registry) or the box. For each
element, the value is read from its property sets, then its quantity sets; the
result is the element id and an optional value (nullopt = property absent).

- Copy the value: `QGuiApplication::clipboard()->setText(value)`.
- Isolate similar / Select similar: `scan`, keep the ids whose value equals the
  clicked text, then `run(Isolate)` or `setSelection`.
- Isolate the property: `scan`, keep the ids that have the property, `run(Isolate)`.
- Colour by property (`colourByProperty`):
  1. `scan` the scope.
  2. Elements without the property → `run(RemoveFromView)`.
  3. Elements with the property are grouped by value (QMap, sorted by value).
  4. Each group → `actions_.colour(ids, colour)`; empty value = grey (150, 150, 150),
     other groups use `groupColour(index)` (hue steps of 137°).
  5. `showTemporaryTable` with a Property column (group key), a Count column and
     a Color column. The Color column shows the viewer's group colour of each row
     (passed by `object_colours`); the table does not repaint the viewer.
- Colour the box by property: as above with `box_only`; an empty box is reported
  in the status bar.

## Known limits

- `scan` is synchronous: it reads every element of the scope on the GUI thread.
  On large models this blocks the UI for a moment. A cache in `PropertyCatalog`
  or a background pass is the next step if it is too slow.
- `colour` is called once per value group (each call refreshes the viewport).
- Colours use the user paint layer: they replace a user colour already on the same objects.
- The temporary table is not persisted: a new « Colour by property » replaces it.

## Not yet verified

- Nothing has been compiled or run on the device for this change.
