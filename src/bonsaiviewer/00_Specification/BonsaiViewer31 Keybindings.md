# BonsaiViewer31 - Keybindings

## Introduction

The main functions of the viewer must have keyboard shortcuts.
A default list of shortcuts is provided with the software. Each user can change them in the Settings.
Shortcuts can be exported and imported, to deploy the same configuration on a fleet of PCs, to make shortcuts uniform across QWERTY and AZERTY keyboards, and to keep the configuration when the software version or the PC changes.

## Specification

### Default Shortcuts

- Every main function of the viewer has a shortcut.
- A default list of shortcuts is provided with the software.
- The editor offers a reset of one shortcut, or of all shortcuts, to its default value.

Default list:

- Enter the search bar: CTRL+SPACE
- Hide: H
- Show: S
- Show All: ALT+S
- Invert View: CTRL+I
- Restore View: CTRL+R
- Undo View: CTRL+Z (see BonsaiViewer02, Undo View)
- Section Plane: CTRL+S
- Remove Section: no default shortcut
- Hide Section: no default shortcut
- Suspend Section: CTRL+D
- Select Everything: CTRL+A
- Toggle Visibility: no default shortcut
- Color: C
- Remove Color: no default shortcut
- Transparency: T
- Box functions:
  - Set Box: CTRL+B
  - Add to Box: CTRL+N
  - Remove From Box: CTRL+,
  - Isolate Box: CTRL+J
  - Hide Box: CTRL+K
- Fit View: CTRL+F
- Perpendicular View: CTRL+P

Toggle Visibility has no default shortcut. Its behaviour: if every selected element is shown, they are hidden; otherwise they are shown.

Other functions already in the viewer, kept with their shortcut (not in the default list above):

- Isolate Selection: SHIFT+H
- Measure Distance: CTRL+SHIFT+L
- Measure Area: CTRL+SHIFT+A
- Measure Volume: CTRL+SHIFT+V
- Fly Mode: SHIFT+F

The previous shortcuts K (section tool) and SHIFT+K (remove section) are replaced by Section Plane (CTRL+S) and Remove Section (no default). ALT+H (Show All) is replaced by ALT+S.

Section functions use the viewport's existing toggles: Hide Section switches the section gizmos hidden, Suspend Section switches the section planes suspended.

### Shortcut Editor

In the Settings, the Keybindings tab offers:

- The list of functions with their current shortcut.
- Click on a shortcut, then type the new key combination to record it.
- A filter box on the function list.
- Conflict detection: a combination already used by another function is refused, a message names that function, and the previous shortcut is kept.
- A Reset button per function, and Reset All. A reset is refused when the default shortcut is used by another function.
- An empty shortcut is allowed: the function then has no shortcut.
- Rows are tall enough (36 px) for the key editor and the Reset button.
- After the editable functions, the table lists the **fixed keys**, greyed and marked "Fixed": keys handled directly by the 3D view or a panel, not editable yet. Alt+H (former Show All), Alt+X (X-Ray), K / Shift+K (section tool, clear planes), Del (remove plane or last point), L / A / V (measure length, area, volume), Esc (exit tool or fly mode), F (focus selection), Home (view all), P (perspective / orthographic), X / Y / Z and Shift+X / Y / Z (standard views), W A S D Q E Shift (fly mode), Ctrl+C / Ctrl+V (copy / paste facets in Filters).

### Export/Import

- The export saves the keybindings only, in a JSON file (format `bonsai-shortcuts`, version 1).
- The shortcut is stored as typed: the file is tied to one keyboard layout (one export for QWERTY, one export for AZERTY).
- The import replaces the current keybindings entirely with those of the file. A function missing from the file loses its shortcut. A file where two functions use the same shortcut is refused, and nothing changes.

### Storage

- The bindings are kept in the user's settings (group `shortcuts`), and read back at start.

## Open Subject

- Make the fixed keys of the 3D view editable (they live in ifcviewer/ViewportWindow), and refuse an editable shortcut that collides with them.
