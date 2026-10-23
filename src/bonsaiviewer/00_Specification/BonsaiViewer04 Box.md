# BonsaiViewer04 - Box

## Introduction

A box is a temporary set of objects, used to quickly isolate or hide groups. The current box and the slots live for the session only and are purged when a model closes. 
A box therefore allows setting aside a set of objects in order to build a comprehensible view. Boxes make it possible to gather heterogeneous objects.

## Specification

### Box

- **Selection → Box** — replaces the current box with the current selection.
- **Add to Box** — adds the selected elements to the current box.
- **Remove from Box** — removes the selected elements from the current box.
- **Isolate Box** — isolates the view on the current box objects.
- **Hide Box** — hides the current box objects in the active view.
- **Box Storage** — dropdown: saves the current box to slot 1 to 3, or isolates the view on a saved slot without touching the current box.
	- The idea is to provide a few extra slots to save our searches. It is a temporary storage.

## Open Subject
