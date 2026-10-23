<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer00 - Existing Implementation

## Introduction
This document describes the viewer as it exists in the code today: the shell (ribbon, docks, status bar), each implemented panel, and the dialogs. It is the reference of the current behaviour, extracted from the source. The "Coming soon" panels are placeholders, not implemented features.

## Shell

### Window
- QMainWindow with dockable panels (nested, tabbed, grouped dragging).
- Title bar: project file name, modified marker on unsaved changes.
- Central widget: the 3D viewport (see Viewport panel).
- Left docks: Models, Spatial Hierarchy. Right docks (tabified): Properties, Layers, Stored Views, Search and Query, Spreadsheet, Audit, Clash, Issues. Only Properties shows by default.
- Status bar: mode label, selection label, GPU memory warning (persistent streaming shortfall after 3 s), performance stats (optional), progress bar (indeterminate then 0-100 during loads).

### Ribbon
Four tabs; each page holds grouped buttons.

**Home**
- PROJECT: New Project, Open Project, Open Cloud, Open Recent (dropdown with recent list, clear, stale entries dropped), Save Project.
- MODELS: Add Model, Sync From Cloud.
- SETTINGS: Settings.

**Navigate**
- CAMERA: Set Home, Go Home, View All, View Selected.
- ORIENTATION: Plan, Front, Side, Align Object (coming soon), Perspective/Ortho toggle.
- MODE: Fly, Section.

**Inspect**
- SELECTION: Hide, Isolate, Show All, Invert.
- MEASURE: Distance, Area, Volume.

**Panels**
- DATA: toggles Models, Spatial, Layers, Properties.
- QUERY: toggles Views, Search, Sheets, Audit.
- COLLABORATE: toggles Clash, Issues.

### Keyboard Shortcuts
Ctrl+Shift+L Distance, Ctrl+Shift+A Area, Ctrl+Shift+V Volume, Shift+F Fly, K Section toggle, Shift+K Clear section, H Hide selected, Shift+H Isolate selected, Alt+H Show all.

## Viewport Panel
The central 3D view (wgpu-based ViewportWindow in a window container).
- Mouse navigation preset, from Settings: Blender / Rhino / Revit / Web button mappings; selection stays on LMB except the Web preset.
- Tools: Length (LMB points, Backspace removes last), Area (LMB add, Alt+LMB single triangle), Volume (click or box-select objects), Fly mode (WASD/QE, Shift+F), Section plane toggle and clear.
- Measures read out in a HUD text; volume draws per-object labels.
- Selection: click picks an object; selection feeds the Properties panel and the volume readout.
- Visibility commands: hide, isolate, show all, invert visibility.
- Standard views (plan/front/side), home position, view all / view selected, projection toggle.
- Background: solid colour from the theme setting "Viewport Background", live on theme change.
- Federated false origin: guessed from the first model added to an empty session, applied to the render transform; models' coordinate operations and transformations are composed per model.

## Models Panel
Tree of the federation: groups (nested, any depth) and models, columns Name / Memory / Visibility (eye).
- Click on the eye toggles visibility of a group or model; click on a model row makes it the active model.
- Drag and drop: models and groups move between groups; a group cannot be dropped into itself or a descendant.
- Context menu on empty space: Add Group. On a group: Toggle Visibility, New Subgroup, Rename Group, Move to Parent (root or any valid group), Remove Group.
- Context menu on a model: View Selected Model(s) (frame camera on them), Rename, New Group, Move to Group, Save To Cloud (only when the model has a cloud source), Save As To Cloud, Load/Unload Model (free or restore GPU memory, model stays in the federation), Remove Model.
- Memory column shows per-model GPU footprint or "unloaded".
- Panel settings button opens the Model Settings dialog.
- Add Model dialog: LOCAL (Add IFC File — geometry and data, Add IFC Database, Add Geometry — .ifcview or .rdbview cache), CLOUD (Add From Cloud), TOOLS (Convert IFC File to Database, Export Geometry Database), with hover descriptions.

### Model Settings Dialog
- Federation tab: Federation Unit (length unit of the federation), Federated False Origin (XYZ, Z rotation in degrees).
- Model tab: selected model georeferencing readout (Georeferenced, Coordinate Operation, Project/Map Unit, Easting, Northing, OrthogonalHeight, X axis, Rotation DD/DMS, Scale, Factors XYZ), and a Model Transformations table (per model: translation/rotation values in Local or Global frame).

## Spatial Hierarchy Panel
Tree of the IFC spatial structure: Site / Building / Storey / Space, with Name, Long Name (storey: right-aligned elevation), and a visibility eye.
- Siblings sorted by name, natural/numeric, case-insensitive.
- Eye click toggles the visibility of the node.
- Context menu: Expand/Collapse Subtree, Expand All, Collapse All.
- Name column width adjustable; expanded by default.

## Properties Panel
Shows the currently selected object, rebuilt on each selection change.
- Entity header: class icon, IfcClass, PredefinedType.
- Sections, each collapsible with its state remembered: Attributes (key/value list), Relationships (key/value with pointer icon), Properties (property sets as groups of key/value), Quantities (quantity sets likewise).
- Properties and Quantities have a filter toggle: a text field filters sets by name or rows by name/value, case-insensitive; placeholders when empty or no match.

## Settings Dialog
Tabs: Interface, Keybindings (placeholder), Graphics, Connectors, About. Modal; OK applies.
- Interface: Navigation preset combo (Blender/Rhino/Revit/Web); Theme (Dark/Light/Custom, with per-colour Pick editors in Custom mode).
- Graphics: General (Geometry Library path, Show Performance Stats, Backface Culling), Loading (Void Limit, Deflection Tolerance, Angular Tolerance, Min Pixel Radius, Motion Min Pixel Radius, LOD1 Pixel Threshold, HiZ Occlusion, HiZ Resolution).
- Connectors: installed connectors with a Settings button (JSON-RPC open_settings; "not available" when the connector opts out).
- About: description, license, third-party licences.

## Connectors
External connector processes (newline-delimited JSON-RPC 2.0), discovered from manifests in the user connectors directory; registry launches them lazily, routes async calls with result/error callbacks, shuts them down on exit. Used by Open Cloud / Sync From Cloud / Save To Cloud.

## Project Commands
- New Project, Open Project (file dialog), Open Project (direct path), Open Cloud Project, Sync From Cloud, Save Project, Save Project As, Save Cloud Project, Save As Cloud Project, Save (asks dialog).
- Recent Projects list persisted; every successful open/save is remembered.

## Placeholder Panels
TodoPanel-based, title + "Coming soon": Layers, Stored Views, Search and Query, Spreadsheet, Audit, Clash, Issues.
