<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer40 - 2D View

## Introduction

A dedicated **interactive 2D view**, synchronized with the 3D view. It shows the **3D model in orthographic projection** — a top view of the whole model, or a **cut section** at an arbitrary plane — in a floating window. Clicking in the 2D view selects the object under the cursor, and the selection is shared with the 3D view in both directions.

The 2D view is distinct from the **Minimap** (BonsaiViewer39, small overview in the top-right corner), from the **drawings contained in the IFC** (BonsaiViewer19) and from the **background plans** (BonsaiViewer36, 3D view only).

## Specification

### 2D window

- The 2D view opens as a **floating window** (dockable), from the Window menu (BonsaiViewer01) or a ribbon button.
- **One 2D window at a time**: opening the command again focuses the existing window.
- **At open, the window is empty**: nothing is displayed until the user picks a display mode.
- Navigation inside the window is **independent from the 3D camera**: pan with the middle mouse button, zoom with the wheel. Navigating in the 2D view does not move the 3D camera.

### Display modes

Two modes, chosen with a control in the 2D window:

- **Top projection** — the whole model in orthographic top view (like the Minimap, BonsaiViewer39, but as a full interactive window).
- **Section** — a cut through the model at an arbitrary plane, in **cut view**: the elements above the cutting plane are removed and the cut is shown in plan.

### Section plane controls

- A panel (in the 2D window or a small attached toolbar) holds **sliders** for the position of the cutting plane and an **orientation** selector: horizontal plane at height Z, or vertical plane along X or Y.
- The slider sets the offset along the chosen axis; the 2D view updates live while the slider moves.

### Interaction and synchronization

- **Left click** on an element in the 2D view **selects the object** under the cursor; the selection is **shared with the 3D view** in both directions (selecting in the 3D view highlights the element in the 2D view, and vice versa).
- **Right click** opens the **same context menu as the 3D view** (select, isolate, hide, colour…), applied to the object under the cursor.
- Clicking on empty space follows the 3D view behaviour (deselect).

### Relations with the other views

- **Minimap (BonsaiViewer39)** — small static overview in the top-right corner with the short level name; the 2D view of this spec is the full interactive window.
- **Background plans (BonsaiViewer36)** — displayed in the 3D view only; they do not appear in the 2D view.
- **IFC drawings (BonsaiViewer19)** — drawings contained in the IFC stay governed by BonsaiViewer19.

## Open Subject

- Relation with the dedicated 2D view of the IFC drawings (BonsaiViewer19): same window as this 2D view, or a separate one?
- Section plane orientation: axis-aligned planes only (X / Y / Z, current choice), or an arbitrary angle?
- Does the 2D view follow the active filters (BonsaiViewer06) and the active box (BonsaiViewer04), like the 3D view?
- Should the 2D window show a scale bar and a north arrow?
- Performance: projection / section of very large models (progressive rendering, simplification)?
