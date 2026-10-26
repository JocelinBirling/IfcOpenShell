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
- **Apply table to box**: the Box toggle of the Table panel lists only the
  elements of the current box. The toggle is saved with the table, and the
  table updates when the box changes.
- **Count** column: the number of elements of the row. It is 1 per row until
  the grouped table (part C).
- **Color** column: a random colour per row, shown in the cell and applied to
  the objects of the row in the viewer. Colours are regenerated when the table
  is evaluated or its columns change.

The grouped table comes next (see Open Subject).



### List Of Table
As the Filter spec, we need a new panel that list the various kind of table that we can create.
We can save them, and dispose them in a tree. We can import them. 
- **Add a new table** — opens the Table Creation Window.
- **Save the Table** — saves the table configuration (columns, order, colours, calculations).
- **Load the Table** — reloads a saved configuration and applies it to the current objects.
- **Create a folder** 

By right clicking, you can open some function, add Folder or Table, delete, or run table.

### Table panel

- The panel « Table » is docked at the **bottom** of the window, opened
  with the **Table** button of the ribbon.
- Tables are created and loaded in the **Tables** panel, not here.
- Toolbar: **Modify** (the creation window of the current table), **Save**
  (save the table in the Tables panel), **Column** (add a column at the right
  end), **Refresh** (evaluate again, for instance after loading a model),
  **Export** (CSV).
- Next to the toolbar: the table name, its number of elements and of
  models.



### Table functions


- **Export table to csv** — exports the displayed data to a CSV file.
- **Add Columns** — opens the Add Column window.
- **Apply table to box** — the table takes only the elements of the active box.

### Table Creation Window

Each table has:

- A name.
- A description — a text description of the table.
- A filter — a selection of elements to use in the table: an **IDS
  specification**, edited exactly like the selection of a filter layer
  (facets, methods of the IDS fields, preview, "Isolate in the view").
  The specification is evaluated on **all the loaded models**; the table
  has one row per element.
- A grouping method — the table can be grouped by one or more properties; for example by entity and then by level.

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
- A method of concatenation.

Only one column can be added at a time. An element without the property
shows an empty cell.

Some specific data can also be added:

- **Count element** — counts the number of elements the row contains.
- **Color** — gives a random colour to the row; all the objects of the row share that colour. If an object is displayed, its colour switches in the viewer.

Columns can also be modified with the window.

### Table interactions

- **Click on a row** — selects the object(s) of the row (several rows with
  Ctrl / Shift).
- **Double-click on a row** — isolates the object(s) of the row.

### Columns interaction

With a left click on a column header, the following functions appear:

- **Modify Column**
- **Remove Column**
- **Add Column to the Right** — the new column is added right of the
  clicked column (the toolbar **Column** adds it at the right end)
- **Left Drag Click to move the column**

### CSV export

The CSV file follows the system's number format so it opens as columns in
a spreadsheet: with a "," decimal separator the fields are separated by
";", otherwise by ",". The file is UTF-8.

### Grouped Table
Some entities can be grouped together.

- **Text data** — the text data is concatenated.
- **Numeric data** — sum, average, minimum, maximum, and count.
- **Mixed data** — numbers are summed, texts are concatenated; any other case returns "ERROR".

By default, text data is concatenated and numeric data is summed; this can be changed for each column.
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

## Open Subject

Phases 2:

- Save / Load the table, several tables.
- Apply table to box.
- Count element and Color columns.
- Grouped table (grouping, concatenation / aggregation methods).

Phase 3:

- Relations (spatial relation).
- Calculated columns (BonsaiViewer23).
