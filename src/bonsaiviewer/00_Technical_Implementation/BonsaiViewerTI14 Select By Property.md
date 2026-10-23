<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI14 - Select By Property

Spec source: `../00_Specification/BonsaiViewer14 Select By Property.md`

## What exists today

`modules/properties` (Panel, View) shows the properties of the selected
object as a key-value table. No context menu on rows.

## Building blocks used

- B05 IfcDataAccess + PropertyIndex — inverted indexes: value → ids
  (same value across models), property name → ids (elements having the
  property).
- B01 SelectionService — "Select similar" (selection replaced, view
  unchanged).
- B08 ActionPipeline — "Isolate similar", "Isolate the property",
  "Colour by property" (auto-colour by distinct values).
- B15 ContextMenuBuilder — the two context menus.
- B04 AppearanceService — colour application.

## New code

- Menu wiring in `modules/properties/View` via B15:
  - On a **value**: Copy the value (QClipboard), Isolate similar, Select
    similar.
  - On a **property name**: Isolate the property, Colour by property.
- No new engine code: everything is a combination of B05 inverted index
  lookups + B01/B08 calls. This fiche is the proof that the bricks pay off.

## Key flows

Isolate similar: property key + value → B05 ids sharing that value →
B08.isolate. Colour by property: ids grouped by distinct value of the
property → one generated colour per group → B04.

## Technical questions

- Equality of values: string comparison of the formatted value, or typed
  comparison (numeric tolerance)? Proposal: typed when both are numeric,
  string otherwise.
- Scope: across all open models (spec says so for Colour by property);
  confirm the same scope for Isolate / Select similar.
