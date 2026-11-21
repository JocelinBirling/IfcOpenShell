
# BonsaiViewer22 - Easy Load File

## Introduction
The idea is to ease the opening of file:
	- Drag and drop file or folder
	- Easier File Explorer
	- Save the last 3 default path

This spec also carries the "Easier model loading" theme of the BonsaiViewer99 ideas backlog (default folder, favorite places, better explorer, Open Recent restricted to `.ifcfed`, and a clear UI message that big models should be converted before opening).

## Specification

### Clear guidance for big models
Even with the additions below, adding a model is still not easy enough for everyone. The UI must make it clear that, for big models or a slow computer, the file should be converted before being opened:
	- The Add IFC Files / Open dialogs show a hint (label or tooltip) that large IFC files open faster once converted to `.ifcview` (single model) or `.ifcfed` (federation), and that the viewer writes/reads the `.ifcview` cache automatically next to the IFC when the folder is writable.
	- The conversion path (Mass Conversion / Save Project As `.ifcfed`) is reachable from the same dialogs or the File menu, one click away.

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

### Default folder (from BonsaiViewer99)
When the file-add dialog opens, it opens by default in a folder the user can choose. This folder is changed in the settings (software parameters), separately from the 3 default paths above: the chosen folder is the starting directory of every file dialog (Add IFC Files, Open Project, Save Project As, Import Document), while the 3 default paths stay as quick links in the explorer.

### Favorite places (from BonsaiViewer99)
The user can save favorite places; they are stored in the side (a Favorites section of the file explorer / a side panel next to the dialog), so they are always visible in the explorer window and reachable in one click, without typing a path.

### Better explorer window (from BonsaiViewer99)
The file explorer must be better than the plain system dialog: a "Look in" top bar that accepts a pasted path (already required above), favorite places and default paths visible in the side, and a clear display of the `.ifcview` / `.ifcfed` conversion state of each listed file (converted cache available or not).

### Open Recent — .ifcfed only (from BonsaiViewer99)
Open Recent only saves `.ifcfed` files. Opening or saving any other file (single `.ifc`, cloud item without a local `.ifcfed`) does not add an entry to the Open Recent list.

## Open Subject

## Implementation notes (to validate)

### Status
- Drag and drop onto the 3D view: implemented in `modules/easy_load/View.cpp` (`EasyLoadView::handleDrop`), including the folder tree option (`createFolderTree`). Not yet wired into `MainWindow` (see below).
- "Look in" with a pasted path: implemented in `components/FileDialogPaths.cpp` (`acceptPastedPaths`), called for Add IFC Files, Open Project and Save Project As. The "Look in" combo is made editable; Enter on an existing folder or file path navigates there. Only the non-native Qt dialog has this bar (the Windows native dialog does not).
- Default paths (up to 3, persisted): implemented — editable in Settings > Paths ("Default path 1..3" with Browse…), stored in `ViewerSettings` (`paths/default_paths`) and added to the sidebar of the non-native file dialogs (`components::applyDefaultPaths`). The automatic fill from the 3 most frequent roots of recently loaded files is still not implemented.
- Default folder (BonsaiViewer99): implemented — Settings > Paths > "Default folder" (`ViewerSettings::defaultFolder`, key `paths/default_folder`); `components::applyDefaultPaths` sets it as the starting directory of Add IFC Files, Open Project, Save Project As, the BCF and the conversion dialogs (all the dialogs that already call `acceptPastedPaths`).
- Favorite places stored in the side: partially implemented — the default paths appear in the sidebar ("side") of the non-native dialogs; a separate Favorites section of the explorer is still open.
- Clear guidance for big models (convert to `.ifcview` / `.ifcfed` first): implemented in the Add Model window — the conversion-tools button group is titled "Easy load for big models or small computer" (`modules/models/AddModelDialog.cpp`); the loader already reads/writes the `.ifcview` cache automatically (see "Conversion to .ifcview" below). Still open: a hint inside the Add IFC Files / Open Project dialogs themselves.
- Open Recent `.ifcfed` only (BonsaiViewer99): not implemented — `modules/project/RecentProjects.cpp` (`RecentProjects::add`, fed by `MainWindow` on `projectOpened` / `projectSaved`) currently records every opened/saved path; it must filter to `.ifcfed` files only.

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

### Conversion to .ifcview
- A dropped IFC that has no .ifcview yet is converted: the drop uses the same load path as Add IFC Files (`addModelFromPath` → `SceneLoader`), and the loader always reads and writes the .ifcview cache (`SessionState`: `setShouldReadIfcView(true)`, `setShouldWriteIfcView(true)`).
- The .ifcview is written next to the IFC (same folder, same name) once the model has finished loading, on a background thread. If it already exists, it is read instead of the IFC geometry.
- Limit: the write fails silently when the IFC folder is read-only; the model is then streamed from the IFC at each open.


## Open Subject
