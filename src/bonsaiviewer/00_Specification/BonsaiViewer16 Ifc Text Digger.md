# BonsaiViewer16 - Ifc Text Digger

## Introduction
A text panel must be added at the bottom, in its own tab. It shows the STEP lines of the IFC text file for the inspected elements, with clickable hashtags to navigate through the references between the entities.

## Specification

### Inspect
The panel holds an Inspect function: it starts from the selected elements. For each selected element, the panel displays its line from the IFC text file, which looks like this:

#0223 = IFCWALL(#2,#3,Text)

### Hashtag navigation
Every hashtag is clickable:

- **Click on a reference hashtag** (in the example, #2) — displays the line that defines it.
- **Click on the declaration hashtag of a displayed line** (in the example, #0223) — displays the lines where that hashtag is referenced, for example #3234 = IfcShape(#0223,#03232).

The lines are always sorted in ascending order.
### Reset
At the top left of the panel, a function resets the view and removes all the displayed lines.

## Open Subject
