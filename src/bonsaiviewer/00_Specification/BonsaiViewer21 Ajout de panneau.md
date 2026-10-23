# BonsaiViewer21 - Add Panel

## Introduction
The previous specs define the base tabs of the viewer. It must be possible to add new pages, each one being a complete workspace: a layout of the window with the 3D panel in the center, composed by the user.

## Specification

### Add a Tab
- In the ribbon tabs, the last tab is a small "+".
- Clicking it adds a new tab.
- The new tab is displayed with the 3D panel in the center.

### Compose a Tab
- Around the central 3D panel, the user adds panels picked from the catalogue of the existing panels (Filters, Properties, Models...).
- Each tab keeps its own composition.

### Manage the Tabs
- **Rename** a user tab.
- **Delete** a user tab, with confirmation.
- **Reorder** the tabs by drag and drop.

### Save the Layouts
- In Settings, a button "Save Layouts" saves the current state of all the tabs of the software.
- There is one single saved layout: the button overwrites it.
- At the next opening of the software, the saved layout is used as the default layout.

## Open Subject
The docking mechanics of the panels in a workspace (dock positions, sizes, floating windows) need to be defined.
