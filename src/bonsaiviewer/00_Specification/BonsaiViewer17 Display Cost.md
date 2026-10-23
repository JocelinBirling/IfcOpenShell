# BonsaiViewer17 - Display Cost

**This can not be deployed, as is, and need a lot of work.** 
Is it coherent ?

## Introduction
The idea is to make Bonsai Viewer usable for a cost estimator (economist):
- Prepare rules to build quantity takeoffs
- Enter prices and descriptions on the fly, to build a price base for the building
- Keep the traceability of the quantified elements once we leave the tool

This works together with the Table (BonsaiViewer07): the object selection comes from the table selects, and most of the feature lives at the table level.

## Specification

### Quantity Rules
A rule calculates one or more values for each object of the table.

The values are computed from the geometry loaded in the viewer (same approach as the Measure module), not read from the IFC base quantities.

The rules are a library of reusable functions, so that many different ratios can be computed quickly. Examples:
- Area (m²)
- Length (ml)
- Perimeter of a space, excluding the door openings
- Cheek (reveal) surfaces of an opening (joues)

### Selection From the Tables
The selection of objects comes from the table rows (BonsaiViewer07 selects): selecting a row selects the objects in the 3D view, which is the input for the quantity rules.

### Editable Columns in the Tables
The quantity tables leave two open columns:
- **Price** - a number, entered by the user
- **Description** - a free text

This way a price base for the building is built on the fly.

The two columns are editable in the viewer tables. The entered values are persisted in the project file, and carried over to the exports (so they survive closing the session and can be shared).

### Traceability of the Quantities
The traceability of the quantified elements must survive leaving the tool.

The export format contains the GUIDs of the quantified elements.

A BCF is generated automatically, containing:
- The quantified objects (GUIDs)
- The quantity found
- A screenshot of the objects

## Open Subject
The exact list of rule functions and their formulas (e.g. perimeter excluding doors, cheek surfaces) need to be defined.
The export formats for the price base (CSV, Excel) need to be chosen.

