<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI06 - Filter Rule

Spec source: `../00_Specification/BonsaiViewer06 Filter Rule.md` (closed)

## What exists today

Nothing: no filter panel, no IDS evaluation, no saved rule documents. The
specification is now closed (layer semantics, IDS facets v1, file format).

## Building blocks used

- B05 IfcDataAccess + PropertyIndex — element classes, PredefinedTypes,
  Psets and quantities to evaluate the facets.
- B06 QueryEngine — the IDS facet evaluator plugs in here (entity facet and
  property facet reuse the same property access paths as the search).
- B07 EntitySetStore — "Run on the box" input list.
- B08 ActionPipeline — the actions: isolate / add to view / remove from view
  / colour / transparency / automatic colour by property.
- B12 TreePanelFramework — the filter tree with folders, drag and drop,
  context menus.
- B14 PersistenceService — filter documents (envelope + IDS specs), import
  and export.
- B15 ContextMenuBuilder, B17 PopupService — tree menus, delete
  confirmation.

## New code

- `services/IdsSelectionEngine`: evaluates an IDS specification (v1: entity
  facet IfcClass + PredefinedType, property facet with comparison) against
  B05, returns an element set. The data model follows the IDS structure so
  attribute / classification / material facets can be added later without a
  redesign.
- `services/FilterRunner`: the layer semantics of the closed spec — a
  pending set; specification layers union into it; AND layers reuse the
  selection above; a layer's actions (via B08) apply to the pending set and
  close the group. The worked example of the spec is the acceptance test.
- `modules/filters/`:
  - Panel (B12 instance): tree of folders and filters, add folder / add
    filter, drag and drop, context menus (run, run on box, modify, delete
    with confirmation, rename folder, delete folder).
  - Filter Creation Window (`components/Dialog` base): name, description,
    layer editor — each layer shows its selection (specification picker or
    AND marker) and its action list; layers ordered top to bottom.
- File format: JSON envelope (name, description, layers, actions) embedding
  standard IDS documents for the specifications; saved in the application
  cache via B14; import from / export to file.

## Key flows

Run: filter → FilterRunner walks layers → IdsSelectionEngine evaluates
specifications (input: whole model, or the box via B07) → actions applied
through B08 in order. Modify: reopen the creation window on the document.
Share: export envelope, import on another machine.

## Technical questions

- IDS parsing: ifctester (Python) cannot be linked; the C++ evaluator only
  needs the entity + property facets — validate the IDS serialisation
  format used by other tools (IDS XML) against our envelope before freezing
  the file format.
- Automatic colour by property: colour assignment strategy (fixed palette
  by distinct value) shared with TI14 "Colour by property" — one
  implementation in B08.
