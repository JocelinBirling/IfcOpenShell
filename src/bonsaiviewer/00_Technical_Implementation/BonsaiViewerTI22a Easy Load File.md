<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI22a - Easy Load File

Spec source: `../00_Specification/BonsaiViewer22 Easy Load File.md`

## What exists today

Models are added through the Models panel / ribbon (Add Model, Open Project,
Save Project As). No drag and drop, no path paste, no default paths.

## Building blocks used

- B14 PersistenceService — default paths, parent-folder frequency history.
- B17 PopupService — reload confirmation, folder confirmation.
- SessionState — existing add-model pipeline reused for all load paths.

## New code

- **Drag and drop** on the 3D view: accept `.ifc` files and folders.
  - Several files at once: all are added.
  - Already-loaded file: popup "reload this model?" (yes/no) per file.
  - Non-IFC files silently ignored.
  - Folder: recursive scan for IFC models; before loading, confirmation
    popup "Do you want to open the X models found in this folder?".
- **File dialog path paste** ("Look in"): native dialogs do not accept a
  pasted path. Two options:
  1. Use non-native QFileDialog (`DontUseNativeDialog`) and add a paste
     handling on the directory bar (a file path opens the containing
     folder; a folder path opens the folder).
  2. Keep native dialogs and add the default paths as sidebar shortcuts.
  Recommendation: option 1 for the paste requirement + sidebar shortcuts
  (below) for speed; applied to Add IFC Files, Open Project, Save Project
  As.
- **Default paths**: three persisted paths shown in the dialogs. Filled by
  counting the most frequent parent folders of recently loaded files
  (frequency history in B14, updated at each load); user-editable in the
  software parameters.

## Key flows

Drop → resolve list (files / folder scan) → per-file already-loaded check →
confirmations → existing add-model pipeline (`SessionState` mappings,
progress bar).

## Technical questions

- Recursive folder scan depth and performance on network drives: cap the
  scan (e.g. ignore hidden/system dirs) and run it in a worker with the
  progress bar.
