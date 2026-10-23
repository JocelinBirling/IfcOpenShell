<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI07 - Table

Spec source: `../00_Specification/BonsaiViewer07 Table.md` (closed)

## What exists today

A "Spreadsheet" dock exists as a placeholder panel. No table engine. The
specification is now closed: relations v1 = spatial only, calculated
columns v1 = strict arithmetic (evolution in BonsaiViewer23).

## Building blocks used

- B05 IfcDataAccess + PropertyIndex — cell values: IFC attributes, Psets,
  quantities; the Add Column tree (classes / Psets / quantities with search
  and checkboxes) reads the same index.
- B01 SelectionService — click on a row selects its objects.
- B08 ActionPipeline — double-click on a row isolates its objects.
- B04 AppearanceService — the Color column: one colour per row, shared by
  the row's objects, applied in the viewer.
- B07 EntitySetStore — "Apply table to box".
- B12 TreePanelFramework — the Add Column window tree.
- B14 PersistenceService — table configurations (columns, order, colours,
  calculations) saved / loaded between sessions.
- B15 ContextMenuBuilder — column header menu (modify, remove, add to the
  right, drag to move).

## New code

`modules/table/`:

- **TableModel**: rows = objects (one per object, or grouped); columns =
  configured property columns + count + colour + calculated columns.
  Grouping by one or more properties (entity then level, etc.).
- **ColumnEngine**: AutoConvertTextToNumeric (handles "." and "," decimal
  separators, stores as real number), concatenation method per column,
  mixed-type aggregation rules (text concatenated, numbers summed, other
  cases "ERROR"; per-column override among sum / average / min / max /
  count).
- **RelationEngine (v1, spatial only)**: IfcRelContainedInSpatialStructure
  and spatial IfcRelAggregates; inputs: type, direction (Ascend = toward the
  container, Descend = toward the contained), reference element. Example:
  table of IfcBuildingStorey + SpatialRelation → storey rows with their
  objects.
- **FormulaEngine (v1, strict arithmetic)**: tokenizer + precedence parser
  for `+ - * /`, parentheses, column references; any text operand →
  "ERROR". Designed with an extension point for the conditional logic of
  BonsaiViewer23 (the AST evaluator, not the parser, is where IF lands).
- **CSV export** of the displayed data.
- Table Creation Window: name, description, filter (element selection),
  grouping method. Add Column window: one column at a time, from the
  property tree (B12-based) plus Count element and Color.

## Key flows

Create table → rows resolved (filter, or box via B07) → columns computed →
grouping applied. Row click → B01; row double-click → B08 isolate. Colour
column → B04. Save / load configuration via B14.

## Technical questions

- Large tables: virtualised model required (QAbstractTableModel, no
  upfront full materialisation).
- The spec's Color column "gives a random colour": define it as a generated
  palette colour per row (stable across refreshes, stored in the
  configuration), not re-rolled at each rebuild.
- Row ↔ objects mapping must be many-to-one (grouped rows) and one-to-many
  via relations: keep the mapping in the model, not in the selection
  service.
