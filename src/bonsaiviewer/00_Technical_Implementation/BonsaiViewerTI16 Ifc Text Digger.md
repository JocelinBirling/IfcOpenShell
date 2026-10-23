<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI16 - Ifc Text Digger

Spec source: `../00_Specification/BonsaiViewer16 Ifc Text Digger.md`

## What exists today

`ElementRegistry` resolves `object_id` to `express::base` entities, but the
raw STEP line (with its `#id` hashtag syntax) is not surfaced anywhere. The
IFC file path is known per model through SessionState.

## Building blocks used

- B16 StepTextService — this fiche builds the brick.
- B01 SelectionService — Inspect starts from the selected elements.
- B13 PanelManager — the bottom text panel, in its own tab.

## New code

- `services/StepTextService` (B16):
  - Per model: access to the original STEP text. Two options: keep the
    parsed file open (memory cost) or re-read lines on demand from the
    source file by entity id (ifcparse provides the line lookup). Decide by
    memory profiling; on-demand reading is the safe default.
  - `lineFor(entity_id)` → `#0223=IFCWALL(#2,#3,Text)` as text.
  - `usagesOf(entity_id)` → lines referencing `#id`, via a per-model
    reference scan cached at first use (or built from the parsed entity
    attributes directly, which is cheaper than a text scan).
- `modules/text_digger/` panel:
  - Inspect: for each selected element, display its line.
  - Hashtags as clickable anchors (QTextBrowser with HTML links).
    Click on a reference hashtag shows the defining line; click on a
    declaration hashtag shows the lines where it is referenced. Lines
    always sorted in ascending id order; navigation history (Back).
  - Reset button top-left: clears the view.

## Key flows

Selection → Inspect → line list; hashtag clicks push lines into the view
(sorted); Reset clears.

## Technical questions

- Federation: the panel must prefix lines with the model name when several
  models are open (ids collide across files).
- Very large files: cache usages maps per model, build lazily.
