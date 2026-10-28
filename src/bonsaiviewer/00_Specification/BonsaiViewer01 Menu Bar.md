# BonsaiViewer01 - Menu Bar

## Introduction
The goal is to centralise functions used a few times per session: file opening, saving, and settings. It is the classic ribbon found in all standard software.

- **File**: New, Open, Open Cloud, Open Recent, Add Model, Sync From Cloud, Save, Save As, Settings.
- **Display**: Show/Hide Spaces, Show/Hide Openings.
- **View**: Fly.
- **Window**: enables/disables the viewer panels. One item per panel, with a checkbox.
- **Help**: Documentation, About. Always positioned at the far right.

## Specification

### File
- **New** — File > New: resets the viewer (closes the file, clears the view).
- **Open** — File > Open: opens an IFC file.
- **Open Cloud** — File > Open Cloud: opens a project from the cloud.
- **Open Recent** — File > Open Recent: submenu of the recent projects.
- **Add Model** — File > Add Model: adds a model to the federation.
- **Sync From Cloud** — File > Sync From Cloud: syncs the cloud project.
- **Save** — File > Save: saves the current file.
- **Save As** — File > Save As: saves under a new name or format.
- **Settings** — File > Settings: application settings.

Each File item has an icon (resources `:/icons/`): New `plus-square`, Open `folder`, Open Cloud `cloud-square`, Open Recent `clock-rotate-right`, Add Model `plus`, Sync From Cloud `refresh-double`, Save `floppy-disk`, Save As `floppy-disk-arrow-in`, Settings `settings`.

### View
- **Fly** — View > Fly: toggles the free-fly camera (WASD/QE, Shift+F to exit).

### Display
Show or hide objects; some objects get in the way of visualisation. For example, IfcSpaces take up all the space and mask objects from clicks. The goal is to disable these objects temporarily. They will never be shown while the buttons are active.

- **Show/Hide IfcSpaces** — Display: shows or hides IfcSpaces permanently in the viewer.
- **Show/Hide IfcOpeningElements** — Display: shows or hides IfcOpeningElements permanently in the viewer.
- **Show/Hide IfcZones** — Display: shows or hides IfcZones permanently in the viewer.
- **Show/Hide IfcDrawings** — Display: shows or hides IfcDrawings permanently in the viewer. Showing plans in the viewer still needs to be worked out.

### Window
This part must contain the viewer's graphical customisation options.
- **Window** — Window: enables or disables the viewer panels.
It must contains every panel that can be added to the tabs. 
Each button will be a toggle button, to display or hide the panel in the current tabs. 
- The check mark shows whether the panel is displayed **in the current tab** (workspace). It is refreshed each time the menu opens, so it follows the tab changes, the workspace defaults and the panels closed with their own button.
- Checking an item displays the panel in the current tab; unchecking hides it. The other tabs keep their own layout.





### Help
- **Documentation** — Help > Documentation: opens the documentation.
- **About** — Help > About: shows the licence information.

# Open Subject
@TODO Add an icon to the functions of the other menus (File done).
