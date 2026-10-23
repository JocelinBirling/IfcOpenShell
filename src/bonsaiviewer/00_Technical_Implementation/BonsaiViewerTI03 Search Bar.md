<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI03 - Search Bar

Spec source: `../00_Specification/BonsaiViewer03 Search Bar.md`

## What exists today

A "Search and Query" dock exists as a placeholder panel. No search engine.
The property data path exists through `ElementRegistry` (object id to
`express::base`).

## Building blocks used

- B05 IfcDataAccess + PropertyIndex — the searchable fields and inverted
  indexes.
- B06 QueryEngine — prefix parsing and evaluation.
- B01 SelectionService, B02 ActiveView — isolation of results.
- B07 EntitySetStore — `-box` scope.
- B08 ActionPipeline — default isolate, `-add`, `-rm`.
- B17 PopupService — "No results".

## New code

- `services/PropertyIndex` (part of B05): per model, built lazily on
  `modelGeometryReady`, rebuilt on reload. Default fields: Name, Long Name,
  parent storey Name / Long Name, Materials, model name, IfcClass,
  PredefinedType. Full-search mode walks all Psets and properties.
- `services/QueryEngine` (B06): tokenizer for the prefix grammar
  (`-fs`, `-prop "Pset.name"`, `-case`, `-view`, `-box`, `-add`, `-rm`,
  combinable), returns `std::vector<uint32_t>`. Substring match,
  case-insensitive by default.
- `modules/searchbar/` (UI in the generic top panel, TI05): QLineEdit +
  magnifier button, Enter triggers, rich-text cheat sheet on hover
  (QToolTip), "No results" popup leaves the view unchanged.

## Key flows

Run: text → B06 evaluates against B05 (scope: whole model, active view via
B02, or box via B07) → result set → default action isolates via B08; `-add`
/ `-rm` route the same set to the other pipeline actions.

## Technical questions

- Index size and build time on large federations: build asynchronously with
  progress reporting through SessionState; document memory cost.
- `-prop` syntax: exact Pset.name match only, or fuzzy Pset completion in the
  cheat sheet?
