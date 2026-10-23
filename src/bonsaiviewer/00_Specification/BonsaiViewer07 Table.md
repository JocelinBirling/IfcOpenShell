# BonsaiViewer07 - Table

## Introduction

A table panel can be added at the bottom of the UI. The table presents a list of objects: a row represents one or several BIM objects, and each column represents an IFC property. This can be useful to represent the model as a data table and to export data from models.

## Specification

### Table functions

- **Add a new table** — opens the Table Creation Window.
- **Save the Table** — saves the table configuration (columns, order, colours, calculations).
- **Load the Table** — reloads a saved configuration and applies it to the current objects.
- **Export table to csv** — exports the displayed data to a CSV file.
- **Add Columns** — opens the Add Column window.
- **Apply table to box** — the table takes only the elements of the active box.

### Table Creation Window

Each table has:

- A name.
- A description — a text description of the table.
- A filter — a selection of elements to use in the table.
- A grouping method — the table can be grouped by one or more properties; for example by entity and then by level.

#### Add Column window

Each column has these settings:

- A common name.
- AutoConvertTextToNumeric — True by default: tries to convert text to a number, handling both « . » and « , » decimal separators, and saves it as a real number.
- A method of concatenation.

When we click on « Add Columns », a window opens showing a tree of the available properties (IFC classes, Psets, quantities) with search and checkboxes, in the manner of Bonsai. Only one column can be added at a time.

Some specific data can also be added:

- **Count element** — counts the number of elements the row contains.
- **Color** — gives a random colour to the row; all the objects of the row share that colour. If an object is displayed, its colour switches in the viewer.

Columns can also be modified with the window.

### Table interactions

- **Click on a row** — selects the object(s) of the row.
- **Double-click on a row** — isolates the object(s) of the row.

### Columns interaction

With a left click on a column header, the following functions appear:

- **Modify Column**
- **Remove Column**
- **Add Column to the Right** — new columns are always added to the right
- **Left Drag Click to move the column**

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
