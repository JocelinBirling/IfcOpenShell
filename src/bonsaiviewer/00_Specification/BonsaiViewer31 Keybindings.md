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

### Shortcut Editor

In the Settings, the shortcut editor offers:

- The list of functions with their current shortcut.
- Click on a shortcut, then type the new key combination to record it.
- A search box and a filter on the function list.
- Conflict detection: a combination already used by another function triggers a warning.

### Export/Import

- The export saves the keybindings only, in a file.
- The shortcut is stored as typed: the file is tied to one keyboard layout (one export for QWERTY, one export for AZERTY).
- The import replaces the current keybindings entirely with those of the file.

## Open Subject

- Undo view (CTRL+Z): see BonsaiViewer02, to be validated before adding it to the default list.
