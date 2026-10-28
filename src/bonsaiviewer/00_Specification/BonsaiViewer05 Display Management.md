# BonsaiViewer05 - Display Management

## Introduction

The goal is to describe the overall UI of BonsaiViewer: the main principles for the organisation of the pages.

- A tabs ribbon: switches from one tab to another.
- A specific top panel: present depending on the tab; gives the tab's common functions.
- A generic function bar: always present at the top of the 3D view, provides the basic display functions.
- Side panels to display the data.

## Specification

### Menu bar
The menu bar is always displayed by the software. It sits at the top left of the screen.

#### Tabs Ribbon Bar
A tabs ribbon bar, below the menu bar and spanning the full length, is always present. It holds the workspace tabs (Home, Filter, Table, see BonsaiViewer21) and the Inspect tab.

### Generic Function Bar
A generic function bar is always attached to the top of the 3D view, inside the central area. It stays reachable whatever the tab, so the section, box, colour and selection functions are always at hand. It holds:

- The search bar
- The display functions
- The section functions
- The box functions
- The zoom functions
- The colour functions

The groups are arranged along the bar: the search bar in the left corner, then the display and section groups; the box functions are centred; the zoom and colour groups are justified on the right edge.

The bar is deliberately compact so the 3D view keeps the room:

- icon-only buttons (16 px icons, 24 px high), the label is shown as a tooltip;
- no group titles;
- search bar 130 x 30 px;
- the bar never imposes its width: when the window is narrow, the right-most groups are clipped instead of squeezing the side panels.

The bar is not an overlay: the 3D view is a native window, and a widget drawn over it cannot be transparent. The bar sits above the 3D view and is opaque.

### Specific Function Panel

One specific function panel per tab, giving access to that tab's own functions. The content of each tab's panel is described in each function.

### Panels
Panels are blocks that can be reorganised within the tab. A few placement rules are kept to stay oriented in the project tabs.

Properties is always top right.

Current panels:

- Properties
- Models
- Spatial Hierarchy
- Ifc Hierarchy
- Layers
- Stored Views
- Search and Query
- Filters
- Table
- Tables
- Ifc Text Digger
- Audit
- Clash
- Issues

The default panels of each workspace are defined in BonsaiViewer21. They can be moved afterwards.

## Open Subject

- Content of the specific function panel for each tab.
- Clipped groups on a narrow window: add an overflow menu (») if needed.
