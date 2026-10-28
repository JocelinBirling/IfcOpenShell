# BonsaiViewer06 - Filter Rule

## Introduction

A filter is a set of rules used to get a precise list of elements and apply a list of actions to it.
Example of a basic function: show the IfcWalls and paint the IfcSlabs red.
These filters are saved between sessions and can be used to easily filter the model for an analysis.
The selections follow the IDS standard (buildingSMART) and are evaluated by the C++ port of IfcTester (BonsaiViewer24).

## Specification

### Filters Panel

The panel "Filters" is opened with the **Filters** button of the QUERY group of the ribbon; it is tabbed next to "Search".

It holds a **toolbar** and a **tree**:

- Toolbar: **Filter** (add a filter), **Folder** (add a folder), **Run**, **Run on the Box**, **Import**, **Export**. New filters and folders go into the selected folder (or the folder of the selected filter). Run and Run on the Box need a selected filter.
- Tree: folders, sub-folders, filters. While the tree is empty, a hint explains how to create or import a filter.
- Drag and drop moves filters and folders in the tree.
- Double-clicking a filter runs it on all the loaded models.

Context menus (right click):

| On | Items |
|----|-------|
| Empty space | Add Folder, Add Filter, Import Filters…, Export All Filters… |
| A folder | Add Folder, Add Filter, Rename…, Delete (with confirmation), Import Filters Here… |
| A filter | Run, Run on the Box, Modify…, Export…, Delete (with confirmation) |

### Save and share

- The whole tree is stored in the application cache (`filters.json`), so it survives closing the session.
- **Export…** writes one filter, **Export All Filters…** writes the whole library (folders included). Both are plain `.json` files, so rules can be shared between people.
- **Import** reads either file; the application recognises a single filter or a library from its content. Imported filters land in the selected folder and get new ids, so nothing clashes with the existing tree.

### IDS engine

The selections are evaluated in the viewer by the C++ port of IfcTester (`ports/ifctester`), on the IFC files the viewer already has open. Nothing has to be installed (BonsaiViewer24, IDS engine).

### Run a filter

- **Run** evaluates the filter on **all the loaded models**, by default. The layers run on each model in turn.
- **Run on the Box** restricts the candidates to the elements of the current box, in every loaded model. An empty box is refused with a message.
- A filter always starts from an **empty view**: everything is hidden right before its first action, then its actions build the view. A filter that only adds the walls to the view shows the walls only. If a selection fails, the view is left untouched.
- **Isolate in the view** replaces what is visible, but keeps the elements already shown by the previous models of the same run.
- A run never creates an undo point (CTRL+Z, see BonsaiViewer02).
- The status bar shows the progress and a summary (layers run, models, elements affected); an error opens a message.

### Filter Creation Window

**Add Filter** and **Modify…** open the creation window. It holds:

- a **name** (required) and a **description**;
- on the left, the **layers** (run from top to bottom), with Add, Remove, Up and Down;
- on the right, the **selected layer**: its **Selection** above, its **Actions** below (the separator can be moved).

The window adapts to the screen size.

#### Selection of a layer

The **Type** of the selection is either:

- **IDS specification**: the layer selects its own elements, or
- **AND, same elements as the layer above**: the layer has no selection of its own. Not available on the first layer.

An IDS specification is a list of **facets**; an element is selected when it passes **all** of them. "+ Add" adds a facet, "Remove" removes the selected one. Selecting a facet shows its configuration page.

- **Entity facet** (exactly one per specification):
  - the **IFC class**: one class, a list of classes or a pattern on the class name;
  - **Include the subtypes** (checked by default): IDS matches the class exactly, so without it IfcWall does not select IfcWallStandardCase;
  - the page shows the classes covered and how many elements of the loaded models they hold;
  - the **PredefinedType**, proposed from the schema for the chosen classes;
  - a class unknown to the schema, or a pattern matching no class, only shows a warning.
- **Property facet** (any number):
  - the **property set** and the **property** (Psets and quantity sets), proposed from the elements of the entity in the loaded models;
  - **Browse properties…** opens a picker laid out like the Properties panel (one group per property set, element counts, a few values, a search field); double-clicking a property fills both fields;
  - the **cardinality**: required (must exist and match), optional (must match when present), prohibited (must not exist);
  - the **value**, with the values found in the model and their element counts.
- **Attribute facet**: the attribute name (Name, Description, Tag…), the value, the cardinality.
- **Classification facet**: the system (empty = any system), the reference, the cardinality.
- **Material facet**: the material, layer, profile or constituent name or category (empty = any material), the cardinality.
- **Part of facet**: the relation (any, aggregation, group / system, spatial container, nesting, void / fill), the parent class (with or without its subtypes), its PredefinedType, the cardinality.
- The property facet also has a **data type** (IFCLABEL, IFCREAL…; empty = any).
- **Copy / paste**: Ctrl+C / Ctrl+V on the facet list, or its context menu (Copy facet, Copy all the facets, Paste facets). The clipboard holds IDS XML, shared with the Tables and the IDS panel (BonsaiViewer24). A pasted entity facet is refused when the selection already has one.

#### Methods of an IDS field

Every IDS field uses the same editor. A row of buttons switches the method; what was typed is carried over to the new method.

| Method | Meaning | IDS |
|--------|---------|-----|
| Value (default) | one exact value; empty = no constraint | simple value |
| List | one of several values: checkable values from the model (with element counts) or the schema, free additions, paste of a list (lines, `;` or `,`), All / None | xs:enumeration |
| Pattern | "starts with", "contains", "ends with", or a free regular expression; the generated pattern and a test field are shown | xs:pattern |
| Range | a minimum and/or a maximum, each included or not; decimal or integer | xs:min/maxInclusive, xs:min/maxExclusive |
| Length | a number of characters: minimum and/or maximum (equal = exact) | xs:length, xs:minLength, xs:maxLength |

Methods offered per field:

| Field | Value | List | Pattern | Range | Length |
|-------|:-----:|:----:|:-------:|:-----:|:------:|
| IFC class | ✓ | ✓ | ✓ | | |
| PredefinedType | ✓ | ✓ | ✓ | | ✓ |
| Property set, Property | ✓ | ✓ | ✓ | | ✓ |
| Property value | ✓ | ✓ | ✓ | ✓ | ✓ |

#### Preview

Below the selection, the preview counts the elements the layer selects in the active model; it updates while editing (**Test** forces it). **Isolate in the view** isolates these elements in the viewer.

#### Actions of a layer

A layer has zero, one or several actions, applied in order. Selecting an action shows its parameters (colour, transparency, property). Several actions can be added to the same layer, so "isolate" and "colour" can be done in one layer.

#### Saving

The filter is checked before it is saved:

- blocking: no name, no layer, a first layer of type AND, no entity class, an incomplete field (empty list, pattern, range or length), a property facet without property set or property, an automatic colour without property, no action in the whole filter;
- warning only: layers whose selection no later action uses (they do nothing).

### Layer semantics

The layers run from top to bottom, with a pending set of elements:

- A layer with a **specification** and no action **adds** its elements to the pending set (union).
- A layer of type **AND** adds nothing: it reuses the selection of the layer above.
- The **actions of a layer** apply to the pending set: the union of the selections accumulated since the last action, plus the layer's own specification if it has one. After the actions run, the pending set is emptied and a new group starts.
- An **AND** layer placed right after an action layer applies its actions to the same elements as the layer above: several actions on the same selection, without duplicating the specification.
- A group whose specifications match nothing stays empty: its actions do not fall back on the previous group.
- The whole list of layers runs once per loaded model; the pending set is separate for each model.

Example (the view starts empty):

| # | Selection | Actions | Effect |
|---|-----------|---------|--------|
| 1 | specification: IfcWall | — | pending = all walls |
| 2 | specification: IfcSlab | — | pending = walls + slabs (union) |
| 3 | AND | Apply Color (red) | walls + slabs are shown and painted red; the group closes |
| 4 | AND | Change the transparency 50 % | same elements as layer 3 become half transparent |
| 5 | specification: IfcSpace | Add to the view | a new group starts: the spaces are shown too |

### Actions

- **Isolate in the view**: replaces what is visible, but keeps the elements shown by the previous models of the run. Inside one model, with several Isolate layers, the last one wins.
- **Add to the view**
- **Remove from the view**
- **Apply Color**: adds the elements to the view, then paints them like the Paint bucket of the ribbon (same colour, removed by Remove Color).
- **Change the transparency**: a percentage of transparency: 0 % = opaque, 100 % = invisible (30 % leaves the elements 70 % opaque). A base transparency of the model (for example windows) is kept.
- **Automatic Color**: adds the elements to the view, then each distinct value of the chosen property gets its own colour (elements without the property are grey). The property can be chosen with **Browse properties…**, on all the loaded models.
- **Activate/Deactivate**: once activated, the elements are always displayed. Their colour and selection cannot be modified.

### Filter file format

A filter is one JSON document (the envelope):

- `format` (`bonsaiviewer-filter`), `version`, `name`, `description`;
- `layers`: for each layer, `and` (AND type), `ids` (the selection) and `actions` (`type` and its parameter: `color`, `percent`, or `pset` + `property`);
- each selection is stored as a **standard IDS document** (buildingSMART), built by IfcTester, so specifications are interoperable with other IDS tools.

A library (Export All Filters) is one JSON document with `format` (`bonsaiviewer-filter-library`), `version`, `folders` and `filters`.

The envelope itself is a simple proprietary JSON; only the specifications it contains follow the IDS standard.

### Performance

**Diagnosis (October 2026).** The selection runs in the IfcTester worker (Python), which opens each IFC itself with `ifcopenshell.open`. The worker kept only one model in memory, so a run on several models re-read every file from disk at each run, and the editor preview (active model) evicted the others again. With models of 200 to 475 MB, this reading dominates the run time. The worker also holds a second copy of models the viewer already has open, and tests the facets element by element in Python.

**Step A — done.** The worker keeps every model it opened, up to `IFCTESTER_IPC_CACHE_SIZE` files (environment variable, 8 by default), least recently used dropped first. A file changed on disk is reopened. Cost: the worker memory grows with the number of models. On the viewer side, the EXPRESS id → object id map is built once per model, and Remove from view runs in linear time.

**Step B — done (October 2026, C++ port of IfcTester, BonsaiViewer24).** Run the selection in the viewer, in C++, on the IFC file it already has open; the worker would only build and parse the IDS XML. This means re-implementing the facets the editor supports:
- Entity: exact class or with subtypes, PredefinedType (inherited from the type);
- Property: property set and property inherited from the type, cardinality required / optional / prohibited;
- Values: simple, enumeration, XSD pattern, range, length, data type, conversion to SI units like IfcTester.

The Python worker is no longer used; Step A only applied to it.

## Open Subject

- Property set / property completion on all the loaded models (BonsaiViewer26 Property Selection).
- Reusable named lists of values.
- Importing third-party IDS files as filter layers (BonsaiViewer24 IDS).
- Colour and visibility are separate: impose a colour without imposing visible elements, and impose a set of visible elements. To validate.
- Auto-completion in the Property fields is validated by a click, not applied on its own when a filter opens.
- Performance step B: C++ selection engine (see Performance).



@TODO, Always acivate filter, to color element everytime.
@TODO Clean the window, it's not clear enough, and some panel are always showing, it's messy.
