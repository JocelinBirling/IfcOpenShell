<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI16 - Ifc Text Digger

Spec source: `../00_Specification/BonsaiViewer16 Ifc Text Digger.md`

## Where it lives

- `modules/text_digger/Panel.h/.cpp`: `TextDiggerPanel`, a `components::Panel`
  (bottom dock, tabified with the Table panel, hidden at start-up). Toolbar
  (Reset, Inspect), a status line, a `QTextBrowser` with links opened by the
  view (`setOpenLinks(false)`).
- `modules/text_digger/View.h/.cpp`: `TextDiggerView`, built in `setupLoader`
  after the selection service.
- `MainWindow`: panel created in `setupPanels`, toggle in the Panels ribbon,
  menu entry `text_digger`.

## Building blocks used

- `SelectionService::selectionIdsAsVector()` — the selected elements.
- `ElementRegistry::findBasicElementInfo()` — `ifc_id` (the STEP number) and
  `session_model_id` of each element.
- `SceneLoader::filePath(session_model_id)` — the IFC text file of the model.

## Reading the file

`forEachRecord(path, visit)` streams the file line by line. A record starts at a
line beginning with `#` and ends at the first text ending with `;` (records
spanning several lines are joined with a space). Header and footer lines are
skipped. The visitor returns `false` to stop early.

- Inspect: one pass per file, collecting the wanted declaration ids; the pass
  stops once all of them are found.
- Click on a reference `#N`: pass until the record whose declaration is `#N`.
- Click on a declaration `#N`: full pass, keeping the records other than its own
  declaration that contain the token `#N` (`#N(?!\d)`, so `#12` does not match `#123`).

## Display

`linkify(text, file)` escapes the record and turns every `#N` into a link:
`ifcdecl:<file>:<N>` when it is the declaration at the start of the line,
`ifcref:<file>:<N>` otherwise. `TextDiggerView::follow` reads the link back.

The shown lines are kept in `std::map<(file index, line number), text>`, which
gives the ascending order.

## Known limits

- Each reference click reads the whole file (on demand, as chosen). On files of
  several hundred MB this takes a while; an index is the next step if needed.
- The search matches the token in the whole record text, so a `#N` inside a
  string literal would also match.
- Lines of a closed project are dropped (`projectReset`, `projectOpened`).

## Not yet verified

- Nothing has been compiled or run on the device for this change.
