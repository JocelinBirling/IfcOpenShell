<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer25 - Chart Viewer

Status: draft, to validate

## Introduction

BonsaiViewer can build **charts** and **dashboards** from the loaded IFC models. A **chart** is a configured visualization — a chart type, fields bound to IFC properties or attributes, an optional IDS filter (BonsaiViewer24), a sort order and a colour palette — evaluated on the loaded models. A **dashboard** is a page composed of charts arranged in rows. Charts and dashboards are managed in a tree, saved with the project, and a dashboard can be exported to PDF.

Clicking a chart element selects or isolates the underlying objects in the 3D view, so a dashboard is also a navigation tool over the models (same interaction model as the Table panel, BonsaiViewer07).

Related specs: BonsaiViewer01 Menu Bar, BonsaiViewer02 Display Function, BonsaiViewer06 Filter Rule, BonsaiViewer07 Table, BonsaiViewer21 Add Panels, BonsaiViewer24 IDS, BonsaiViewer26 Property Selection, BonsaiViewer27 CtrlZ, BonsaiViewer31 Keybindings, BonsaiViewer35 External Table, BonsaiViewer36 Add Drawing Background.

## Specification

### Delivery phases

- **Phase 1**: the chart types below, the Dashboards tree, the Chart Edition window, the Dashboard Edition window, the Dashboard View (composition + interactions), PDF export, save / load with the project.
- **Phase 2**: the Map chart with a user-loaded SVG (see Map chart); the auto-generation of the SVG files is specified in BonsaiViewer43.

### Tab organization

- A new ribbon tab **"Dashboard"** is added to the default tabs (BonsaiViewer21). Default layout:
  - **Dashboards** panel: whole left;
  - **Dashboard View**: center;
  - **3D view**: whole right, so the selection sync between charts and 3D is visible by default.
- Tabs are added / renamed / deleted / reordered, and layouts are saved and restored at start-up, exactly as in BonsaiViewer21. The Dashboards panel and the Dashboard View are panels of the catalogue and can be added to any user tab.

### Panels

#### Dashboards panel (tree)

- Docked **left** of the **Dashboard** tab; opened with the ribbon **Dashboards** button or from the Window menu (BonsaiViewer01); **visible at start-up**.
- Tree of **folders**, **dashboards** and **charts**, each with its own icon so the three node types are recognizable at a glance.
- **Empty state** (no node yet): "No dashboard yet. Right-click to add a folder, a dashboard or a chart."
- Toolbar buttons:
  - **Add Folder** — creates a folder inside the selected folder (or at the root). Always enabled.
  - **Add Dashboard** — creates a dashboard and opens the Dashboard Edition window. Always enabled.
  - **Add Chart** — creates a chart and opens the Chart Edition window. Always enabled.
  - **Import** — imports a node from a JSON file (see Save / load). Always enabled.
  - **Refresh** — re-evaluates the charts of the displayed dashboard. **Disabled when no model is loaded.**
- Right-click menus:
  - on a **chart**: **Edit** (Chart Edition window), **Duplicate**, **Export (JSON)**, **Delete**;
  - on a **dashboard**: **Edit** (Dashboard Edition window), **Show**, **Export (JSON)**, **Delete**;
  - on a **folder**: **Add Folder**, **Add Dashboard**, **Add Chart**, **Rename**, **Export (JSON)**, **Delete**;
  - on the **root / empty space**: **Add Folder**, **Add Dashboard**, **Add Chart**, **Import**.
- Click behaviors on a tree node:
  - **single left-click**: selects the node;
  - **double left-click**: on a chart, shows the chart in the Dashboard View; on a dashboard, shows the dashboard in the Dashboard View; on a folder, expands / collapses it;
  - **single right-click**: opens the context menu above.

#### Dashboard View

- Central panel of the **Dashboard** tab. Shows the selected dashboard, or a single chart when a chart is double-clicked in the tree.
- Toolbar buttons:
  - **PDF Export** — exports the displayed dashboard (or single chart) to a PDF file. **Disabled when the view is empty.**
  - **Refresh** — re-evaluates the charts. **Disabled when no model is loaded.**
  - **Edit Dashboard** — opens the Dashboard Edition window of the displayed dashboard. **Disabled when a single chart is displayed.**
- Empty states:
  - nothing selected: "Double-click a dashboard or a chart in the Dashboards panel.";
  - dashboard without charts: "Right-click to add a row and a chart.";
  - no model loaded: each chart shows "No model loaded".
- Right-click menus:
  - on **empty space** of the view: **Add Row**, **Color the 3D Objects Like the Chart**, **Edit Dashboard** (dashboard only);
  - on a **row**: **Set Layout** (1 chart / 2 charts / 2:1), **Add Chart**, **Delete Row**;
  - on a **chart cell**: **Show Chart Full**, **Edit Chart**, **Export PDF**, **Remove from Dashboard** (the chart stays in the tree).
- Click behaviors:
  - **row** — single left-click: selects the row (highlighted); double left-click: no action; right-click: row menu;
  - **chart cell** — single left-click: selects the cell; double left-click: **Show Chart Full**; right-click: cell menu.

### Chart types

The catalogue mirrors the chart types of **Apache Superset**: Superset is the reference for the chart types and the interactions — the ideas are copied, not the tool itself (Decided 2026-11-22). The rendering engine is still open (Open Subject).

- **Correlation — Bubble Chart**: a metric across three dimensions in a single chart (X axis, Y axis, bubble size); bubbles of the same group share a colour.
- **Distribution — Histogram**: frequency / count of values within ranges (bins); shows the shape, central tendency, spread and outliers of the data.
- **KPI — Big Number**: a single metric front-and-center.
- **KPI — Gauge Chart**: progress of a metric toward a target; the dial position is the value, the end of the gauge is the target.
- **Part of Whole — Pie Chart**: proportions of a whole.
- **Part of Whole — Tree Map**: hierarchical data, the value is the area.
- **Table — Pivot Table**: row-by-column spreadsheet-like view of the data with aggregated metrics.
- **Table — Table**: data summarized along two axes (e.g. tasks by status and assignee).
- **Text Chart**: a fixed text displayed as a chart (dashboard annotation).
- **Filter Chart**: a dashboard-level filter (like the Power BI slicers) — it filters the data **before** it is sent to the other charts of the dashboard for refresh, so one can play on the data of the whole dashboard. One filter acts on **all the charts of the dashboard** (Decided 2026-11-22).
- **Map — SVG Map** (phase 2): an SVG plan loaded by the user, whose elements are linked to the IFC objects — see Map chart.

Every chart has a **title**; every dashboard has a **title and a subtitle**. The subtitle is on one line under the title; if it is too long, the full text is shown on **hover** (tooltip).

### Chart Edition window

Opened by **Add Chart** or **Edit** on a chart. Fields:

- **Name** — required, unique within its folder.
- **Description** — free text.
- **Chart type** — dropdown of the catalogue above; changing the type keeps the fields that still apply.
- **Fields** — one input per field of the chart type (e.g. X, Y, Size for the bubble chart; Metric for the big number; Rows, Columns, Values for the pivot table). Each field accepts either:
  - a **property**, chosen with the standard property field (BonsaiViewer26: Pset + Property inputs with completion on all the loaded models, and **Browse…**);
  - or an **attribute / category**: **IfcClass**, **IfcSite**, **IfcBuilding**, **IfcBuildingStorey**, picked in a category dropdown.
  - A numeric metric field accepts only properties that evaluate to a number (AutoConvertTextToNumeric, BonsaiViewer07); otherwise the field shows a warning.
- **Filter** — an optional **IDS specification** (BonsaiViewer24), edited exactly like the selection of a filter layer. Default: no filter = all the loaded models.
- **Sort** — ascending / descending on the main value.
- **X axis title**, **Y axis title** — free text; empty = automatic, derived from the field.
- **Colour palette** — dropdown of the predefined palettes.
- Validation and buttons: **OK** is disabled until the name is filled and every required field of the chart type is set. **OK** saves the chart in the tree and closes the window; **Cancel** discards the changes.

### Dashboard Edition window

- **Name** — required.
- **Title** — shown at the top of the dashboard view.
- **Subtitle** — one line under the title; a long subtitle is truncated and shown in full on hover.
- **OK** saves; **Cancel** discards.

### Dashboard composition

- A dashboard is a **list of rows**; each row holds charts with one of these layouts:
  - one chart on the full width;
  - two charts side by side (50 / 50);
  - one large chart (2/3) and one small (1/3).
- Charts are added to a row by **drag & drop from the tree** onto the row, or with **Add Chart** in the row's right-click menu (a picker lists the charts of the tree).
- Rows, and charts inside a row, can be reordered by drag & drop.
- The layout is deliberately limited to these row types: versatile, but simple to compose and to implement.

### Chart interactions (Dashboard View)

For every chart, whatever its type:

- **Hover** on an element (bar, bubble, slice, cell…): tooltip with the value of the element.
- **Single left-click** on an element: selects in the 3D view the objects that compose this value. Ctrl / Shift extend the selection to several elements.
- **Double left-click** on an element: isolates in the 3D view the objects of this value (if the element was not selected, only it is isolated).
- **Right-click** on an element: menu **Select the Elements**, **Isolate the Elements**.
- **Right-click** on an empty zone of a chart: **Color the 3D Objects Like the Chart** — applies the chart's colours to the objects in the 3D view (same rule as the Color column, BonsaiViewer07).
- Selection sync is bidirectional: selecting objects in the 3D view highlights the corresponding chart elements (BonsaiViewer02, BonsaiViewer07).

### Scope

- Default scope of every chart and dashboard: **all the loaded models**.
- Like the Table panel (BonsaiViewer07), a run option **Run on Box** evaluates only the elements of the current box; it is a run option, not saved with the chart.

### Save / load configuration

- The **tree (folders, dashboards, charts) and all their configurations are saved with the project** and reloaded at start-up, like the tables (BonsaiViewer07) and the external documents (BonsaiViewer35).
- **Export (JSON)** on a node writes one `.json` file holding the node and its children (charts with their full configuration, dashboards with their layout); **Import** reads it back. Only configurations are stored, never the model data.
- On project open, charts are re-evaluated when a dashboard or a chart is displayed (or on **Refresh**), not at load time.

### PDF export

- **PDF Export** writes the displayed dashboard (title, subtitle, all rows and charts) to a `.pdf` file chosen in a save dialog. A single chart can be exported the same way from its cell menu (**Export PDF**). The PDF is a static rendering, not interactive.

### Map chart — SVG (phase 2)

- A chart of type **Map** displays an **SVG loaded by the user** — chosen with a **Browse…** file field in the Chart Edition window, or picked among the SVG documents of the **External Documents** window (BonsaiViewer35); those SVGs can be imported by hand or **generated from the loaded models** (BonsaiViewer43). Several SVGs can be loaded (e.g. one per storey), and several storeys can be merged into one SVG.
- Each element group carries the **IFC GlobalId** of the object it represents — `id` (with `use-element-guids`, BonsaiViewer43) and always `ifc:guid` — plus `ifc:name` and `class` = the IFC class. Cut shapes also carry `class="cut"`, the others `class="projection"` (the fill styles of the IfcOpenShell SVG serializer). Any object class, not only spaces: SVGs generated by BonsaiViewer43 can contain walls, slabs and any class selected at generation, cut at the chosen cut height.
- When the map is displayed, an option shows the **IfcName / LongName** at the centre of each shape for readability — read from `ifc:name` for any class, or rendered by the serializer itself for spaces (`print-space-names`).
- The user simplifies / edits the SVG by hand, outside BonsaiViewer (remove duplicate spaces, etc.).
- Interactions are the standard chart interactions: hover shows the value, single left-click selects, double left-click isolates, right-click menu **Select the Elements** / **Isolate the Elements**. A shape whose GlobalId matches no loaded object is greyed and selects nothing.
- The **auto-generation of the SVG plans** (one SVG per storey, spaces as shapes) is specified separately: **BonsaiViewer43**.

## Performance

- Charts are evaluated on demand (display of a dashboard / chart, **Refresh**), never at project load; the result is cached until the models change, the chart configuration changes, or **Refresh** is pressed.
- Evaluation aggregates the elements in a single pass over the filtered scope; long evaluations show a progress bar in the status bar.

## Decisions

- Decided (2026-11-22): Apache Superset is the reference for the chart types and the interactions — the ideas are copied, not the tool itself; the rendering engine stays open.
- Decided (2026-11-22): a new **Dashboard** tab is a default tab, and the Dashboards panel and the Dashboard View are also in the panel catalogue, addable to any user tab (BonsaiViewer21).
- Decided (2026-11-22): the dashboard layout is a list of rows limited to 1, 2 (50/50) and 2:1 splits.
- Decided (2026-11-22): the tree and the chart / dashboard configurations are saved with the project **and** can be exported / imported as JSON.
- Decided (2026-11-22): the Filter Chart acts globally on all the charts of the dashboard.
- Decided (2026-11-22): the auto-generation of the SVG plans is a separate spec (BonsaiViewer43); BonsaiViewer25 only loads user-provided SVGs.

## Open Subject

- Rendering engine of the charts: native Qt charts vs embedded web view (QWebEngine + a JS charting library). Superset stays the UX reference either way.
- Performance limits: maximum number of elements per chart evaluation (histograms, pivot tables) — to set with the first benchmarks.
