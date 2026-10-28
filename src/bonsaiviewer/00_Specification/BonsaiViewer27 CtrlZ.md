# BonsaiViewer27 - Undo (CTRL+Z)

## Introduction

CTRL+Z is a simple step-back on the view. It is not a general undo of the application: it only restores the display and colour state of the viewer.

## Specification

- **Scope**: visibility, colour and transparency of the objects.
- **History**: 5 states at most. The oldest is dropped when a new one is added.
- **Stored before**: Show, Hide, Show All, Invert View, Restore the View, Paint, Remove Colour, Transparency, and the isolations from the Properties, Table, Box and Model panels.
- **Not stored**: section planes, camera, selection, filter runs, table runs.
- **Reset**: the history is emptied when a model is loaded or closed.
- **No history**: CTRL+Z does nothing.

The full rules are in BonsaiViewer02 (Undo View). The default shortcut is listed in BonsaiViewer31 (Keybindings).

## Open Subject
