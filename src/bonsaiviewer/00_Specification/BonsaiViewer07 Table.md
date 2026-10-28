# BonsaiViewer07 - Table

## Introduction

A table panel can be added at the bottom of the UI. The table presents a list of objects: a row represents one or several BIM objects, and each column represents an IFC property. This can be useful to represent the model as a data table and to export data from models.

## Specification

### Delivery

The table is delivered in phases. **Phase 1 (done)**: one table at a time,
the Table Creation Window (name, description, IDS selection), the columns
(property, name, class, GlobalId, model) with the Add / Modify Column
window, the header menu, moving columns by drag, click / double-click on
rows, and the CSV export.

**Phase 2, part A (implemented, to build and test)**: the Tables panel
(folders, tables, drag and drop, run, import and export of one table),
Save of the current table, and Load (run) of a saved table.

**Phase 2, part B (implemented, to build and test)**:
- **Run on Box**: the Tables panel runs a table on the box: only the elements
  of the current box are listed. It is a run option, not saved with the table;
  the table updates when the box changes.
- **Count** column: the number of elements of the row.
- **Color** column: a random colour per row, shown in the cell and applied to
  the objects of the row in the viewer. Colours are regenerated when the table
  is evaluated or its columns change.

**Phase 2, part C (implemented, to build and test)**: the grouped table (see
Grouped Table below): grouping keys in the Table Creation Window, aggregation
per column in the Add Column window.

**Phase 3 (implemented, to build and test)**: sorting and filtering on the
columns (see Sort and Filter below).



### List Of Table
As the Filter spec, we need a new panel that list the various kind of table that we can create.
We can save them, and dispose them in a tree. We can import them. 
- **Add a new table** — opens the Table Creation Window; the table is saved at
  once in the selected folder (or at the root).
- **Save the Table** — saves the table configuration (columns, order, grouping,
  aggregations, calculations). Done from the Table panel.
- **Run the Table** — loads a saved configuration and evaluates it on the loaded models.
- **Run on the Box** — the same, keeping only the elements of the current box
  (not saved with the table).
- **Create a folder** — folders are optional: a table can sit at the root.
- **Import / Export** — one table per file.

By right clicking, you can open some function, add Folder or Table, rename,
delete, export, run, or run on the box.

### Table panel

- The panel « Table » is docked at the **bottom** of the window, opened
  with the **Table** button of the ribbon.
- Tables are created and run in the **Tables** panel, not here. The Table
  panel has no creation action: it modifies, saves and evaluates the loaded table.
- Toolbar: **Modify** (the creation window of the current table), **Save**
  (save the table in the Tables panel), **Column** (add a column at the right
  end), **Refresh** (evaluate again, for instance after loading a model),
  **Export** (CSV).
- Next to the toolbar: the table name, its number of elements and of
  models.



### Table functions


- **Export table to csv** — exports the displayed data to a CSV file.
- **Add Columns** — opens the Add Column window.
- **Run on Box** — in the Tables panel, next to Run: the table takes only the
  elements of the active box.

### Table Creation Window

Each table has:

- A name.
- A description — a text description of the table.
- A filter — a selection of elements to use in the table: an **IDS
  specification**, edited exactly like the selection of a filter layer
  (facets, methods of the IDS fields, preview, "Isolate in the view").
  The specification is evaluated on **all the loaded models**; the table
  has one row per element.
- A grouping — the checked columns are the grouping keys: the elements sharing
  their values form one row (see Grouped Table). Count and Color columns cannot
  be keys.

A new table starts with two columns: Name and Class.

#### Add Column window

Each column has these settings:

- The data: a **Property** (property set or quantity set), or the element's
  **Name**, **Class**, **GlobalId** or **Model** (the IFC file name).
- The property, chosen with the standard property field
  (BonsaiViewer26 Property Selection): Pset and Property inputs with
  completion on all the loaded models, and **Browse…**.
- A common name — empty = automatic (the property name).
- AutoConvertTextToNumeric — True by default: tries to convert text to a number, handling both « . » and « , » decimal separators, and saves it as a real number.
- An aggregation (used by a grouped table): Default, Concatenate, Sum, Average,
  Min, Max or Count (see Grouped Table).

Only one column can be added at a time. An element without the property
shows an empty cell.

Some specific data can also be added:

- **Count element** — counts the number of elements the row contains.
- **Color** — gives a random colour to the row; all the objects of the row share that colour. If an object is displayed, its colour switches in the viewer.

Columns can also be modified with the window.

### Table interactions

- **Click on a row** — selects the object(s) of the row (several rows with
  Ctrl / Shift).
- **Double-click on a row** — isolates the object(s) of the selected rows.
  If the clicked row is not selected, only that row is isolated.
- **Right-click on a row** — menu **Isolate**, same rule as the double-click.

### Columns interaction

With a right click on a column header, the following functions appear
(a left drag on the header moves the column):

- **Modify Column**
- **Remove Column**
- **Add Column to the Right** — the new column is added right of the
  clicked column (the toolbar **Column** adds it at the right end)
- **Left Drag Click to move the column**

### Sort and Filter (phase 3)

Each column header shows, right of its name, a small **arrow** and a small **funnel**.

- **Sort** — clicking the arrow sorts the table on the column: first click
  alphabetical order (arrow down, A to Z), second click reverse order (arrow up,
  Z to A), third click back to the table order. One sorted column at a time;
  the arrow of the sorted column is highlighted. Numbers sort by value and
  before texts, texts are case-insensitive, empty cells stay at the end.
- **Filter** — clicking the funnel opens the list of the distinct values of the
  column, each one with a check box ("(empty)" for empty cells), a search field
  to narrow the list, **All** / **None** (on the listed values), **Clear Filter**
  and **Apply**. The table keeps the rows whose value is checked. Several
  columns can be filtered: a row must pass all of them. The funnel of a
  filtered column is filled.
- The summary shows "12 of 40 row(s)" while a filter hides rows.
- Selection, isolation and the CSV export follow the displayed rows: the export
  writes the rows shown, in the shown order.
- Sorting and filters are not saved with the table; they are cleared when the
  columns change (add, modify, remove, move) or another table is loaded.
- Clicking the arrow or the funnel does not start a column drag; the rest of the
  header keeps its behaviour (right click menu, left drag to move).

### CSV export

The CSV file follows the system's number format so it opens as columns in
a spreadsheet: with a "," decimal separator the fields are separated by
";", otherwise by ",". The file is UTF-8.

### Grouped Table
Some entities can be grouped together.

- **Text data** — the text data is concatenated.
- **Numeric data** — sum, average, minimum, maximum, and count.
- **Mixed data** — numbers are summed, then the texts follow after a semicolon (for example `12; text A; text B`).

By default, text data is concatenated and numeric data is summed; this can be changed for each column.

- **Grouping keys** — in the Table Creation Window, the checked columns are
  the keys: the elements sharing the same values of these columns form one row.
  A key column shows the shared value.
- **Aggregation** — each column (Add / Modify Column window) has an aggregation:
  Default, Concatenate, Sum, Average, Min, Max, Count (number of non-empty
  values). Count and Color columns have none.
- **Count** — the number of elements of the row.
- Rows of a grouped table keep the viewer behaviour: selection, isolation and
  Color apply to all the elements of the row.

### Relations
A relation can be added to the table, taking three inputs:
- the relation type
- the relation direction (Ascend or Descend)
- the reference element
It adds the elements that are related to the reference element. For example, to get a table with the levels and their objects, make a table of IfcBuildingStorey and add a SpatialRelation to add all the objects linked to each IfcBuildingStorey.

The first version supports the **spatial relation** only: IfcRelContainedInSpatialStructure and the spatial aggregations (IfcRelAggregates). Other relation types (component aggregation, group assignment) are not in the first version.

### Calculated column
A calculated column can be added, holding a simple function to run operations on the table values. 
In a table where we already have the following columns:
| Entity | Width | Height |

we could add a new column called « Volume » with the function « Width x Height », which will produce the multiplication.

The formulas use Excel-style syntax: standard operator precedence, with parentheses; several calculations can of course be chained. The computation stays algebraic: if a value is text, the cell returns "ERROR".

The first version is strictly arithmetic: `+ - * /` with parentheses and references to other columns; any text value returns "ERROR". The evolution of the calculated columns (conditional logic, comparisons, IF) is specified on its own: BonsaiViewer23 - Table Calculated Columns.

### Example tables
At the first start of a version with examples, the Tables panel gets (once, next to the saved tables) an **Examples** folder, open, with tables that show the features. Each description explains what the table shows:
1. **Wall quantities** — one row per wall: Name, Class, Model and quantities; sort, filter, export.
2. **Elements per class** — a grouped table (Class as key) with Count and Color.
3. **Wall areas by type** — two grouping keys, Sum and Average aggregations.
4. **Doors and windows** — an entity facet on two classes; Run on Box.
5. **Space areas** — sort and filter on the net floor area and the model.

The examples are ordinary tables: they can be modified, moved or deleted. They are added when the store holds no table at all (for instance after deleting every table), and once per examples version (setting `examples/tables_version`): deleted examples do not come back while other tables remain. A new version replaces the "Examples" folder at the root.

## Open Subject

Phase 3: done (Sort and Filter).


Phase 4:

- Relations (spatial relation).
- Calculated columns (BonsaiViewer23).




