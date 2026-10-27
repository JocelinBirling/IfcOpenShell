
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
	- If a folder is dropped, search it recursively for IFC models. Before loading, ask the user in one window: "Do you want to open the X models found in this folder?" with the choices "Open only", "Open and create folder tree", "Cancel".
- "Open and create folder tree" recreates the sub-folders as groups in the Models panel, with the same hierarchy. The dropped folder itself is not created as a group. A sub-folder is created only if it holds IFC files, directly or deeper.
- A model already loaded in the view is not reloaded in that case: it is moved to its place in the tree.

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

## Implementation notes (to validate)

### Status
- Drag and drop onto the 3D view: implemented in `modules/easy_load/View.cpp` (`EasyLoadView::handleDrop`), including the folder tree option (`createFolderTree`). Not yet wired into `MainWindow` (see below).
- "Look in" with a pasted path: implemented in `components/FileDialogPaths.cpp` (`acceptPastedPaths`), called for Add IFC Files, Open Project and Save Project As. The "Look in" combo is made editable; Enter on an existing folder or file path navigates there. Only the non-native Qt dialog has this bar (the Windows native dialog does not).
- Default paths (3 most frequent roots, persisted): not implemented.

### Decisions taken
- Only `.ifc` files are loaded (case-insensitive). Other dropped files are ignored silently.
- A dropped folder is searched recursively. If it holds IFC files, one window asks with the count and three choices: Open only, Open and create folder tree, Cancel. A folder with no IFC files is ignored.
- Folder tree: one group per sub-folder (parents created first), the dropped folder is not a group. Files directly in the dropped folder stay at the root of the Models panel. The group of a model is set with `Federation::setModelGroup`; the groups with `Federation::addGroup`.
- The id of a newly added model is the most recent model of the Federation whose source path is the dropped path (`addModelFromPath` returns only a bool).
- A file already loaded (same canonical path) asks "Do you want to reload it?". Yes removes the loaded model, then adds it again.
- Models are added with the same command as Open Recent (`commands::addModelFromPath`).

### Wiring (done, not yet compiled or tested)
- The 3D view does not receive drops: the viewport is a native window (`QWindow` in a window container) and Qt delivered no drag event to the application in test. The drop target is therefore the Models panel.
- `ModelsTreeView` (`modules/models/Panel.cpp`): accepts URL drops and calls the handler set by `ModelsPanel::setFilesDropHandler`. Internal drags (moving models) are unchanged.
- `MainWindow::setupLoader`: sets the handler to `easy_load_view_->handleDrop(urls)`.
- `CMakeLists.txt`: `modules/easy_load/View.cpp` and `View.h` added.

### Open points
- Drops on the 3D view: the viewport may sit in a window container that does not receive drops. To check at the first test.
- Keybindings is also numbered 22 (`BonsaiViewer22 Keybindings.md`): two specs share the number. To renumber.

-If a models has not already be converted, it need to be added to the file.