# BonsaiViewer14 - Properties Panel

## Introduction

In the Properties panel, a right click on a property value or on a property name opens a set of functions to select, isolate or colour the elements that share that value or property. It's a quick way to filter the model, without a big filter rule.

## Specification


## Specification - Ifc Data
We should propose a way to display IfcData. It's in the schema but not in an easy way. 
It should ease the way. 
In the relation

### Relation to display
Level Name
Model Name
Building Name
Site Name

#### Ifc Group 
IfcGroup
IfcSystem
IfcAsset
IfcInventory
IfcStructuralLoadGroup
IfcStructuralResultGroupe

#### IfcRelationShip
IFCRELCONNECTSPORTTOELEMENT : Viens connecter les réseaux en IFC

Ifc REL TO MATERIAL

### Relation as "Properties"
Those relation info can be requested in the the Table or in the Filter Action.
Color automatic by Models Name
Color By IfcSystem Name









## Specification Select By Color


### Right click on a property value

- **Copy the value** — copies the value to the clipboard.
- **Isolate similar** — all the objects with the same property value are isolated in the view.
- **Select similar** — all the objects with the same property value are selected; the view is unchanged.

### Right click on a property name

- **Isolate the property** — all the objects that have this property are isolated.
- **Colour by property** — across all the models, the value of this property is read; the elements sharing the same value are grouped, and each group gets its own distinct colour. A temporary table in the Table panel shows the values, their element counts and the colour of each value in the view (see BonsaiViewer07 Table).
- **Colour the box by property** — same as « Colour by property », but only for the objects in the active box.

### Decisions

- **Equality**: the values are compared as text (the value shown in the panel).
- **Scope**: all the loaded models, for every function, including « Isolate similar » and « Select similar ». « Colour the box by property » is limited to the box.
- **Objects without the property**: they are hidden in the view. An object that has the property but an empty value is coloured grey.
- **Temporary table**: « Colour by property » opens a temporary table in the Table panel (value and count). It is not saved and is not run on the IDS selection; it can be modified, exported to CSV, or isolated by row. Saving it is refused.
- **Quantities**: the right-click functions also apply to the quantity sets (Qto_*), since they are shown in the same way.

## Specification Unified Data
We want to deduce some data from the models, unified the Level Rattachement, or the building.

If can construct some data automaticaly, but it's not actual data from the IFC.
We should present that data somewhere and make the possibility to export.