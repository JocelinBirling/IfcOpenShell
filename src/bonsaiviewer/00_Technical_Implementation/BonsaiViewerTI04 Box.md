<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI04 - Box

Spec source: `../00_Specification/BonsaiViewer04 Box.md`

## What exists today

Nothing: no named element sets exist today. This fiche introduces B07, which
TI03 (`-box`), TI06 (run filter on box) and TI07 (apply table to box) will
reuse.

## Building blocks used

- B07 EntitySetStore — the box itself (this fiche builds the brick).
- B01 SelectionService — source of the objects.
- B08 ActionPipeline — Isolate Box.
- B02 ActiveView — Hide Box.

## New code

`services/EntitySetStore` (B07):

- API: `replaceCurrent(ids)`, `addToCurrent(ids)`, `removeFromCurrent(ids)`,
  `current()`, `saveSlot(n)` (n in 1..3), `slot(n)`, `purgeModel(model_id)`.
- Object ids stored as a dense set; on model close, `purgeModel` removes the
  ids of that model (hook SessionState model removal signals), matching the
  "session only" lifetime of the spec.
- UI: group in the generic top panel (TI05): Selection to Box, Add to Box,
  Remove from Box, Isolate Box, Hide Box, and a Box Storage dropdown
  (save to slot 1-3 / isolate on a slot without touching the current box).

## Key flows

Isolate Box: B07.current() → B08 isolate → B02. Hide Box: B07.current() →
B02 remove. Isolate slot: B07.slot(n) → B08 isolate, current box untouched.

## Technical questions

- Should saved slots also survive within the session across model reloads of
  the same file (ids may change on reload)? Proposal: purge on reload too,
  matching the spec's session-only rule.
