# BonsaiViewer06 - Filter Rule

## Introduction

A filter is a set of rules used to get a precise list of elements and apply a list of actions to it.
Example of a basic function: show the IfcWalls and paint the IfcSlabs red.
These filters are saved between sessions and can be used to easily filter the model for an analysis.
A new panel will be created: « Filters ».

## Specification

### Filter Panel
The panel holds a tree of filters:
- Filter folders
- Folders of folders
- Add a folder
- Add a filter

### Function of Filter Panel
Save the filters:

- The filters are saved as a JSON file, so rules can be shared between people.
- The filters are stored in the application cache, so they survive closing the session.

Load the filters:
- Filters from someone else can be loaded.

Add a filter:
- It opens the Filter Creation Window.

### Filter Function
With a left click on a filter, you can access the following functions:

- **Run a filter** - runs the filter on the whole model.
- **Run on the box** -runs the filter with the active box as input list, instead of the whole model.
- **Modify a filter** - It opens the Filter Creation Windows
- **Delete a filter** - asks for confirmation.

By left drag, you can move the filter in the tree.

### Function of folders

With a left click on a folder, you can access the following functions:

- Rename folders
- Delete folders

By left drag, you can move the folder in the tree.

### Filter Creation Window
When we click on « Add a filter », a temporary window opens. The window holds:
- a name
- a description
- one or more layers

Each layer is split into two parts, and the actions run from top to bottom.

- **The selection part** — reuses the IDS bricks. It holds a specification; a specification can contain several IDS facets and must select all the wanted elements. The first version supports the **entity facet** (one or several IfcClasses, optionally with their subtypes, + PredefinedType) and the **property facet** (Psets and quantities, with comparison). The other facets (attribute, classification, material) are out of the first version; the data model follows IDS so they can be added later.
- **The action part** — the place where the user picks one or more actions, from the list described in the Actions section.

### Layer semantics

A filter always starts from an **empty view**: when it runs, everything is hidden, then its actions build the view. A filter that only adds the walls to the view shows the walls only.

The layers run from top to bottom, with a pending set of elements:

- A layer with a **specification** and no action **adds** its elements to the pending set (union).
- A layer marked **AND** adds nothing: it reuses the selection of the layer above.
- The **actions of a layer** apply to the pending set: the union of the selections accumulated since the last action, plus the layer's own specification if it has one. After the actions run, the pending set is emptied and a new group starts.
- An **AND** layer placed right after an action layer applies its actions to the same elements as the layer above: several actions on the same selection, without duplicating the specification.
- The selection of a layer is **either** an IDS specification **or** AND (chosen in the Selection part of the layer). The first layer cannot be AND.
- Selection layers that no later action uses do nothing: saving the filter shows a warning (not blocking).

Example:

| # | Selection | Actions | Effect |
|---|-----------|---------|--------|
| 1 | specification: IfcWall | — | pending = all walls |
| 2 | specification: IfcSlab | — | pending = walls + slabs (union) |
| 3 | AND | Colour red | walls + slabs are painted red; the group closes |
| 4 | AND | Isolate in the view | same elements as layer 3 are isolated |
| 5 | specification: IfcSpace | Hide | a new group starts: the spaces are hidden |

### Filter file format

A filter is one JSON document (the envelope):
- the name, the description;
- the list of layers with their actions;
- each specification inside is stored in the **standard IDS format** (buildingSMART), so specifications are interoperable with other IDS tools.

The envelope itself is a simple proprietary JSON; only the specifications it contains follow the IDS standard.

### Actions
An action is a modification of the model:

- Isolate in the view — replaces what is visible: with several Isolate layers, the last one wins.
- Add to the view
- Remove from the view
- Add to the view and colour — adds the elements to the view, then paints them like the Paint bucket of the ribbon (same colour, removed by Remove Color).
- Change the transparency
- Add to the view and automatic colour by property — adds the elements to the view, then each distinct value of the chosen property gets its own colour.

## Open Subject
