# BonsaiViewer29 - Model Panel

## Introduction

The Models panel lists the models of the federation. Each model can be acted on as a whole, without selecting its elements one by one. The panel gets three functions: colour a model, isolate a model, and select a model.

## Specification

### Model functions

Right-click on a model in the Models panel opens the menu:

- **Colour Model**: opens the colour dialog and paints every element of the model with the chosen colour. Same behaviour as **Paint** in BonsaiViewer02 (colour and transparency kept separately, removed by Remove Colour or Restore the View).
- **Isolate Model**: shows only the elements of this model and hides the other models. The category filters still apply: a disabled category stays hidden.
- **Select Model**: selects every element of the model. The view is unchanged.

### Rules

- Colour Model and Isolate Model store an undo state (BonsaiViewer02, Undo View).
- Colour Model overrides Clash Filter colouring until a new clash run, as for Paint.
- Restore the View (BonsaiViewer02) removes model colours and shows every model again.
- A model that is not in the view (hidden or not loaded) is ignored by Select Model.

## Open Subject

- Show/Hide Model: not in the first version; to decide whether it is needed beside Isolate Model.
- Select Model with hidden elements of the model: selected or ignored? (see BonsaiViewer28 IfcGroup, which has the same question for groups.)
- Model functions from the menu bar or the keyboard: no shortcut defined yet (BonsaiViewer31).
