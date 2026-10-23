<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI22b - Keybindings

Spec source: `../00_Specification/BonsaiViewer22 Keybindings.md`

Note: the spec's open subject (the concrete default key list) does not block
the architecture below; the list is a content table filled when decided.

## What exists today

Shortcuts are hardcoded scattered QShortcut/QAction calls (Ctrl+Shift+L
Distance, H Hide, etc.). No registry, no editor, no import/export.

## Building blocks used

- B14 PersistenceService — export / import of keybinding documents.
- ViewerSettings — active bindings stored as the user's configuration.

## New code

- `services/ShortcutRegistry`:
  - Every main function registers with a stable id ("measure.distance",
    "view.hide_selection", ...) and a default QKeySequence.
  - Central `sequenceFor(id)` consumed by the actions at creation; the
    registry owns the live QShortcut objects so rebinding applies
    immediately.
  - Conflict detection: a sequence maps to at most one function; assigning a
    used combination warns (spec: warning, not blocking).
- Editor UI in the settings dialog (`modules/settings`):
  - List of functions with current shortcut, search box and filter.
  - Click a shortcut → record mode captures the next key combination.
  - Reset per function and reset-all to defaults.
- Export / import: JSON document (function ids + sequences as typed);
  tied to one keyboard layout (one file for Qwerty, one for Azerty);
  import replaces the entire current configuration.

## Key flows

Register: actions declare their id at construction → ShortcutRegistry
applies the active sequence. Edit: capture → conflict check → save to
ViewerSettings → registry rebinds.

## Technical questions

- Sequences stored as typed (layout-bound) per the spec: keep the native
  text of the QKeySequence; document that a file is layout-specific.
- Existing hardcoded shortcuts migrate to the registry in one sweep; no
  behaviour change expected.
