# BonsaiViewer12 - Ifc Class Tree

## Introduction

A  classification tree completes the existing trees: a panel with the IFC classes of the project, grouping the objects by IFC class. This tree contains all the models of the project.

## Specification

### Ifc Class Tree panel

The tree shows the IFC classes of the project with all their instances in the groups. Only the classes that have instances appear. All the open models are merged in a single tree.

- IfcWall
	- Wall1
	- Wall2
	- Wall3
- IfcSlab
	- Slab1
	- Slab2

The panel is refreshed at each model load.

### Interactions

The standard viewer conventions apply:

- **Click on an instance** — selects the element.
- **Double-click on an instance** — isolates the element.
- **Click on a class** — selects all the instances of the class.

## Open Subject
