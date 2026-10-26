<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI06 - Filter Rule

Spec source: `../00_Specification/BonsaiViewer06 Filter Rule.md` (closed)

## What exists today

Implemented, IDS evaluation delegated to IfcTester (decision: no C++ IDS
evaluator — the "IdsSelectionEngine" below is the IdsBridge + worker):

- `src/ifctester/ifctester_ipc.py` — JSON Lines worker 2.0. A selection is
  a list of facets (`entity`, `property`) whose fields are VALUEs: none,
  simple, enumeration, pattern (XSD), range (min/max, inclusive or not) or
  length (characters).
  Ops: build / parse (facets <-> canonical IDS XML, round-trip exact),
  select, preview (build + select), and introspection for the editor:
  classes (schema of the model), classInfo (subtypes, PredefinedType
  enumeration, element count), properties (psets/properties present on the
  entity's elements), values (distinct values with counts, guessed base).
  Entity facet: `name` is a VALUE (simple, enumeration or pattern; the
  pattern is upper-cased like IDS class names); several classes or
  "include subtypes" = xs:enumeration of the classes and their subtypes
  (IDS matches the class exactly). Keeps the last IFC in memory.
- `services/IdsBridge` — the worker process + the C++ model (`IdsValue`,
  `IdsFacet`, `SelectionFields{facets}`); settings keys `filters/interpreter`,
  `filters/script`.
- `services/FilterStore` — tree in `filters.json` (B14); export / import of
  one filter or of the whole library, as plain `.json` files (the content
  tells them apart: a filter has `layers`, a library has `filters`).
- `services/FilterRunner` — layer semantics; a run starts from an empty view
  (everything hidden right before the first action, so a failed selection
  leaves the view untouched); the two colour actions add the elements to
  the view before colouring them; Run on the box restricts the
  candidates to the box's EXPRESS ids; `activeIfcPath()` shared with the UI.
- `modules/filters/`:
  - Panel + View: toolbar (Filter, Folder, Run, Run on the Box, Import,
    Export), context menus, drag and drop, double-click = run.
  - CreationDialog: layers; per layer a facet list ("+ Add facet") and the
    page of the selected facet; the Selection type is "IDS specification"
    or "AND — same elements as the layer above" (not offered on the first
    layer); a non-blocking warning at save lists the selection layers no
    later action uses; live preview (count in the active model,
    "Isolate in the view" through B08); actions with their parameters
    (the automatic colour has a "Browse properties…" picker on all the
    products of the active model).
  - FacetPages: every IDS field is a ValueEditor. EntityFacetPage (class:
    Value / List / Pattern with schema completion, subtypes and the classes
    they cover, PredefinedType from the schema enumeration, element count);
    PropertyFacetPage (property set, property, value, cardinality, and a
    "Browse properties…" button).
  - ValueEditor: the standard editor of an IDS field. Methods Value
    (default; empty = no constraint), List, Pattern, Range, Length, switched
    by a row of buttons; the typed text is carried over to the new method.
    Each field shows the methods that make sense for it.
  - PropertyPicker: the property sets of the layer's entity laid out like
    the Properties panel (element count, a few values), with a search
    field; double-click fills the property set and the property.

Not done yet: attribute / classification / material / partOf facets (listed
disabled in "+ Add facet"); reusable named value lists; IDS import of
third-party files as filter layers (see BonsaiViewer24 IDS).

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
