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
- Show All: ALT+H
- Invert View: CTRL+I
- Restore View: CTRL+R
- Undo View: CTRL+Z (see BonsaiViewer02, Undo View)
- Section Plane: K
- Remove Section: SHIFT+K
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

Functions that were hard-coded in the 3D view are now in the list, with their former key as default:

- Isolate Selection: SHIFT+H
- Measure Distance: L
- Measure Area: A
- Measure Volume: V
- Fly Mode: SHIFT+F
- X-Ray: ALT+X
- Focus Selection: F
- View All: HOME
- Perspective / Orthographic: P
- Front / Back View: X / SHIFT+X
- Right / Left View: Y / SHIFT+Y
- Top / Bottom View: Z / SHIFT+Z

Fly mode keys, editable, active only in fly mode (they may reuse a key of another function, e.g. S; they must differ from each other). Defaults for QWERTY:

- Fly: Forward / Back / Left / Right: W / S / A / D
- Fly: Turn Left / Turn Right: Q / E (90° per second)
- Fly: Up / Down: R / F

On an AZERTY keyboard, the user sets Z / S / Q / D and A / E in the editor (or imports an AZERTY shortcut file).

The 3D view keeps only the keys of an active mode, listed as "Fixed" in the editor: Esc (exit tool or fly mode), Del / Backspace (remove a section plane or the last length point), Shift (faster in fly mode). The Filters facet list keeps CTRL+C / CTRL+V. The former camera log key (C) is removed: C is Color.

Bindings already saved in the user's settings keep their value: Reset All applies these defaults.

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
- After the editable functions, the table lists the **fixed keys** of an active mode, greyed and marked "Fixed".

### Export/Import

- The export saves the keybindings only, in a JSON file (format `bonsai-shortcuts`, version 1).
- The shortcut is stored as typed: the file is tied to one keyboard layout (one export for QWERTY, one export for AZERTY).
- The import replaces the current keybindings entirely with those of the file. A function missing from the file loses its shortcut. A file where two functions use the same shortcut is refused, and nothing changes.

### Storage

- The bindings are kept in the user's settings (group `shortcuts`), and read back at start.

## Open Subject

- None.
