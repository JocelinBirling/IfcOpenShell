
# BonsaiViewer22 - Easy Load File

## Introduction
The idea is to ease the opening of file:
	- Drag and drop file or folder
	- Easier File Explorer
	- Save the last 3 default path

## Specification

### Drag And Drop
We can select several file in the explorer and drop them in the 3D View.
If the view already contains files or doesn't, it will add them to the view.

Rules:
	- If a dropped file is already loaded in the view, it need to ask if the file need to be reloaded. 
	- Non-IFC files are silently ignored.
	- If a folder is dropped, search it recursively for IFC models. Before loading, ask the user for confirmation: "Do you want to open the X models found in this folder?".

### Easier File Explorer
When you click on:
	-Add IFC Files
	-Open Project
	-Save Project As

Copy/Paste a file path in the top bar. This will directly open the folder of the file.
Paste a folder path in the top bar and this will open the folder.

The top bar is name "Look in" in the windows.

Today, the "Look in" bar is only a dropdown list of folders and does not accept a pasted path. The requirement is that this bar accepts a pasted path: a file path opens the folder containing the file, a folder path opens the folder.

### Define Default path
In the software parameters, you can add 3 files path that will be displayed in the "Explorer Windows". There will always be visible here, in order to get faster to the preferred path.

By default, this path will be completed by the 3 most common root of the file you use.
A "root" is the parent folder of a loaded file. The viewer counts the most frequent parent folders of recently loaded files and fills the default paths with the top 3. This history is persisted in the settings.

## Open Subject
