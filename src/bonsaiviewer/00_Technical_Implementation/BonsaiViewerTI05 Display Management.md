<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI05 - Display Management

Spec source: `../00_Specification/BonsaiViewer05 Display Management.md`

## What exists today

The shell already matches the structure: menu bar to add (TI01), tabs ribbon
(`components/Tabs`), central 3D viewport, dockable panels. What is missing is
the generic top panel and the per-tab specific panels.

## Building blocks used

- B13 PanelManager — panel placement rules, tab composition.
- Hosts the UI groups of TI02 (display functions, colour, zoom), TI03 (search
  bar), TI04 (box) inside the generic panel.

## New code

- `components/GenericTopPanel` — a horizontal ribbon strip attached below the
  tabs bar, holding the groups of TI02 / TI03 / TI04 in fixed slots: search
  bar left corner, then display, box, zoom, colour groups. Always visible.
- `components/SpecificTopPanel` — one instance per tab, populated from a
  description registered by each feature; empty for tabs without specific
  functions. Content of each tab's panel is defined by the feature specs
  (open subject of the source spec, filled progressively).
- Placement rules in B13: Properties always docked top-right; other panels
  free within the tab.

## Key flows

Tab switch → B13 activates the tab's SpecificTopPanel and its dock layout;
the GenericTopPanel is shared and unchanged.

## Technical questions

- Generic panel overflow on narrow windows: define wrapping or elision
  behaviour.
