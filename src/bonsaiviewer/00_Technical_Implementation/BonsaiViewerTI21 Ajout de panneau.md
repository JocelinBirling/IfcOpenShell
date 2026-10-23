<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI21 - Add Panel

Spec source: `../00_Specification/BonsaiViewer21 Ajout de panneau.md`

## What exists today

Tabs exist in the ribbon (`components/Tabs`) but there is a fixed set of
them; panels are dockable but there is no per-tab layout memory and no user
tabs.

## Building blocks used

- B13 PanelManager — this fiche is the main consumer: user tabs, per-tab
  composition, "+" tab.
- B14 PersistenceService — single-slot layout save.

## New code

- B13 extensions:
  - Tab model: system tabs (fixed) + user tabs (created by the "+" tab at
    the end of the ribbon). A new user tab opens with only the central 3D
    panel.
  - Composition: the user adds panels from the catalogue of registered
    panels (the same factories the Window menu uses); each tab keeps its own
    dock layout (QMainWindow::saveState per tab, or one main window per tab
    — decide during implementation; per-tab saveState on a hidden container
    is the lighter path).
  - Management: rename (double-click), delete with confirmation (B17),
    reorder by drag and drop.
- Layouts: Settings button "Save Layouts" serialises the state of all tabs
  (tab list, names, per-tab dock states) into one B14 document; single
  saved layout, overwritten each time; restored at startup.

## Key flows

"+" → new tab, 3D panel centered → user drags panels in → rename/reorder →
Save Layouts → next launch restores.

## Technical questions

- Docking mechanics inside a tab (positions, sizes, floating windows) is the
  spec's open subject: start with Qt dock behaviour as-is and refine.
- Panel instances shared between tabs (one Properties panel in two tabs) or
  per-tab instances? Per-tab instances is simpler; shared state stays in the
  services (B01/B02), so duplication is only visual.
