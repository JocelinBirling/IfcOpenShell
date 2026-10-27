# BonsaiViewer14 - Properties Panel

## Introduction

In the Properties panel, a right click on a property value or on a property name opens a set of functions to select, isolate or colour the elements that share that value or property. It's a quick way to filter the model, without a big filter rule.

## Specification


## Specification - Ifc Data
The IFC relations of an object (where it is, which groups it belongs to) are in the schema but hard to reach. The Properties panel shows them in its **Relationships** section, and lets the user open the related entities.

### Relationships section
Rows, from top to bottom; a row with no value is not shown:

| Row | Content |
|---|---|
| Model | Name of the model the object belongs to (as in the Models panel). Shown even without the IFC data source. |
| Type | The construction type (existing row). |
| Site, Building, Storey, Space | The spatial chain, found by walking up from the container of the object. |
| System | One row per IfcSystem the object is assigned to. |
| Zone | One row per IfcZone. |
| Asset | One row per IfcAsset. |
| Group | One row per other IfcGroup. |

The former « Container » row is replaced by the spatial chain.

```
Relationships
  Model    : Projet_CVC.ifc
  Type     : Gaine rectangulaire 400x200
  Site     : Site principal
  Building : Bâtiment A
  Storey   : R+1
  System   : CVC Soufflage
  System   : Plomberie EF
  Zone     : Zone Feu 2
  Group    : Lot 12
```

#### Groups
- **IfcSystem subtypes**: IfcDistributionSystem, IfcDistributionCircuit, IfcBuildingSystem, IfcBuiltSystem and IfcStructuralAnalysisModel are `System`. IfcZone (a group of spaces) is `Zone`.
- **Other groups**: IfcAsset is `Asset`. Every other IfcGroup (plain IfcGroup, IfcInventory, IfcStructuralLoadGroup, IfcStructuralResultGroup) is `Group`.
- **Label**: the Name of the related entity, or its GlobalId when the Name is empty.
- **Nesting**: only the groups the object is directly assigned to (IfcRelAssignsToGroup). Parent groups are not inherited.

#### Open a related entity
Entities without geometry (IfcSite, IfcBuilding, IfcBuildingStorey, IfcSpace, IfcSystem, IfcZone, IfcGroup…) can be shown in the Properties panel, with their attributes, relationships, properties and quantities.

- A click on the **pointer icon** at the right of a Relationships row opens that entity in the panel. The value text is not underlined and is not clickable. The Model row has no icon.
- A click on a name in the Spatial Hierarchy opens that spatial element in the panel. The System Hierarchy (to be specified) will do the same.
- **← Back**, at the left of the entity header, returns to the previous entity. It is shown only when there is a history. A new selection in the viewport empties this history.
- The panel re-reads the shown entity when the IFC data source of its model opens.
- **Viewport**: unchanged. Opening an entity does not change the selection or the view.

#### Isolate a relation
A right click on a Relationships row (name or value) shows **Isolate**. It isolates, in the viewport, the elements linked by that relation:

| Row | Isolated elements |
|---|---|
| Model | Every element of the model. |
| Type | Every occurrence of the type. |
| Site, Building, Storey, Space | Everything inside: contained elements and spatial children, recursively. |
| System, Zone, Asset, Group | The members of the group. A member that is itself a group adds its own members. Members are not expanded spatially (a zone isolates its spaces, not their content). |

Aggregated parts are always included, since the geometry often sits on them. Only elements with geometry are isolated; when there is none, the status bar shows "No element to isolate". Isolate goes through the action pipeline, so CTRL+Z restores the previous view.

#### IfcRelationShip (to specify)
IfcRelConnectsPortToElement: connects the elements of networks.

IfcRelAssociatesMaterial: material of the element.

### Relations in Table and Filter (to specify)
Those relations can be requested in the Table or in the Filter Action.
- Colour automatically by Model name.
- Colour by IfcSystem Name: the value is the system names joined by "; ", so an object in several systems forms its own value ("CVC; Plomberie"), with its own colour and its own row in the temporary table. Objects without any system are hidden, like objects without the property in « Colour by property ».

### Storage (decision)
The relations are not saved: no cache file, nothing added to the .ifcview. Like the property sets, they are rebuilt on each read from the IFC data source, and are available once that source is open (except the Model row).

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


## Open Subject
Relation to display
Unified Data to display