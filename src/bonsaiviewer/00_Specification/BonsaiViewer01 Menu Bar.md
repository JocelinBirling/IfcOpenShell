# BonsaiViewer01 - Menu Bar

## Introduction
The goal is to centralise functions used a few times per session: file opening, saving, and settings. It is the classic ribbon found in all standard software.

- **File**: New, Open, Save, Save As, Settings.
- **Display**: Show/Hide Spaces, Show/Hide Openings.
- **Window**: enables/disables the viewer panels. One item per panel, with a checkbox.
- **Help**: Documentation, About. Always positioned at the far right.

## Specification

### File
- **New** — File > New: resets the viewer (closes the file, clears the view).
- **Open** — File > Open: opens an IFC file.
- **Save** — File > Save: saves the current file.
- **Save As** — File > Save As: saves under a new name or format.
- **Settings** — File > Settings: application settings.

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
### Help
- **Documentation** — Help > Documentation: opens the documentation.
- **About** — Help > About: shows the licence information.

# Open Subject
