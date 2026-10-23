<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI01 - Menu Bar

Spec source: `../00_Specification/BonsaiViewer01 Menu Bar.md`

## What exists today

The shell is a QMainWindow with a ribbon (four tabs) and dockable panels, but
no menu bar (`MainWindow.cpp`). File operations already exist behind the
ribbon PROJECT group (New, Open, Open Cloud, Save) and settings in
`modules/settings/Dialog`.

## Building blocks used

- B03 CategoryOverrideFilter — Display menu toggles.
- B13 PanelManager — Window menu content.
- B17 PopupService — About dialog.

## New code

`modules/menubar/` with a single `MenuBar.cpp/.h` building four QMenus:

- **File** — reuses existing command objects: New / Open / Save / Save As
  (project module), Settings (settings dialog). No new logic, only wiring.
- **Display** — four checkable QActions, one per category (IfcSpaces,
  IfcOpeningElements, IfcZones, IfcDrawings), bound to B03. Checked state
  persisted in ViewerSettings so the session restores them. IfcDrawings is
  disabled until TI19 lands.
- **Window** — generated from B13: one checkable action per registered panel,
  toggling visibility in the active tab. Rebuilt when panels register.
- **Help** — Documentation opens the docs URL (repo `docs/` build); About
  shows licence info (GPL-3.0) and version.

## Key flows

Display toggle: action triggered → B03.setCategoryEnabled(class, on) →
B03 forces recompute of B02 → viewport refreshes via SessionState
`visibilityChanged()`. Because B03 is the final visibility gate, a hidden
category cannot reappear through isolation, search, or Show All.

## Technical questions

- Menu bar visibility: spec says always displayed; keep it non-hideable in
  B13 layouts.
- IfcDrawings toggle: park as disabled or hidden until TI19 exists.
