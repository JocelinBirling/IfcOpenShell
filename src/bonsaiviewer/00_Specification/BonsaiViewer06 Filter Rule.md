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

- **The selection part** — reuses the IDS bricks. It holds a specification; a specification can contain several IDS facets and must select all the wanted elements. Selections can be combined with a **union** (the elements matching one or the other). Instead of a specification, a layer can hold an **AND** that reuses the selection of the layer above, to apply several actions to the same selection.
- **The action part** — the place where the user picks one or more actions, from the list described in the Actions section.

### Actions
An action is a modification of the model:

- Isolate in the view
- Add to the view
- Remove from the view
- Colour the elements
- Change the transparency
- Automatic colour by property — each distinct value of the chosen property gets its own colour.

## Open Subject
The AND and Union part of the filter need to be discussed.
