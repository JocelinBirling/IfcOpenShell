# Bonsai Viewer PLUS
Welcome to **Bonsai Viewer PLUS**, it's a community build from **Bonsai Viewer**.
Here's the link to [IfcOpenshell](https://ifcopenshell.org/) without whom this project wouldn't exist.


<p align="center">
<img src="./src/bonsaiviewer/00_Images/BonsaiViewerCommunity.png">
</p>

Bonsai viewer is really fast, and federates huges models. 
Bonsai viewer Plus give it several way to deals with thoose models. It give function to cut, select, color, hide and show and manage those view. It give several other function:
- Filter to modify aspect of the models.
- Tables to view data as a table, and to allow export. 
- A Search bar to dig into the models
- Digger to actually see the text of ifc file next to the 3D
- Custom layer to modify the aspect of it
- IDS to manage IDS
- Unified Tree wich gather tree from all models to display an unique one



![](./src/bonsaiviewer/00_Images/BasicView.png)

**It's made with IA.**

## Features

### Common Display

These features are **always present in the display**: they are the base of every model modification. All of them are exposed in the **Menu Bar** (the ribbon), one button group per family.

- **Hide, Show and Isolate**: driven by the selection — hide the selected elements, isolate them (hide everything else), show everything again, invert the visibility.
- **Cut plane**: section planes to cut the model open and inspect what is inside.
- **Box**: This allow to save some objects to construct complex gathering of objects.
- **Color**: paint elements with a colour, or restore their original style.

### A Search bar
You don't know where to search, and you don't know exactly what you are looking for ! That's the perfect place.
It allows to dig in the models and find your interessting data. 


### Filters

Filter rules to analyse the model: selections built from **IDS facets** (class, property, attribute, classification, material, part of), organized in **layers**, and **actions** applied to the result — isolate, add to or remove from the view, colour, transparency, automatic colour, activate/deactivate. Filters are saved between sessions and can be imported and exported as JSON files.

![](./src/bonsaiviewer/00_Images/Filter.png)

You can display data as a color. It allow a better vizualisation of data.

![](./src/bonsaiviewer/00_Images/AutomaticFilterByColor.png)

### Tables

Tables of elements, opened by the tools that produce them: values and counts of a property, colours of an automatic colour, results of a filter. The Table panel shows them even when hidden, and they can be browsed and exported.

![](./src/bonsaiviewer/00_Images/Export_Table.png)

You can do small pivot table that can interact with the 3D viewer.

![](./src/bonsaiviewer/00_Images/Table_Gather_Elements.png)

### Ifc Digger

A microscope on the IFC file itself: explore the raw structure of the model — the spatial hierarchy, the entities, their attributes, relationships and properties. For exploring and learn the Ifc file with a graphic views. 

![](./src/bonsaiviewer/00_Images/Ifc_Digger.png)

### Custom Layer
You can create your own custom windows, to display it as you like.

![](./src/bonsaiviewer/00_Images/Custom_Layer.png)



### IDS
It can create an IDS specification, modify it, and launch it on the loaded models.
The results drive both the view and the Properties panel: colour the objects by IDS result, colour the properties in the panel by their result, and filter the properties to keep only the ones the IDS selects.

![](./src/bonsaiviewer/00_Images/IDS_Validation.png)

## What's Next

### BCF
I plan to introduce BCF management. 

### A better tree management system
There are a lot of tree in the Ifc Schema, IfcGroup, IfcSystem, IfcZone, etc...
I plan to standardize a view to allow for a quick acces to it.

### Clash Management tools
My main part was to create a clash management tools. I have already build it in Pyhon, it need to be cleaned and exported there.

### Calculation in table
We could do some small calculation in the viewer, multiply, add to value together. Nothing to fancy, but it could save a small step from going to excel. 

### Draw small chart
After the table, i could create a smalls charts about the models.

### BSDD Manage
There is probably something to do with the BSDD in here.