# BonsaiViewer05 - Display Management

## Introduction

The goal is to describe the overall UI of BonsaiViewer: the main principles for the organisation of the pages.

- A tabs ribbon: switches from one tab to another.
- A generic top panel: always present, provides the basic display functions.
- A specific top panel: present depending on the tab; gives the tab's common functions.
- Side panels to display the data.

## Specification

### Menu bar
The menu bar is always displayed by the software. It sits at the top left of the screen.
#### Tabs Ribbon Bar
A tabs ribbon bar, below the menu bar and spanning the full length, is also always present. It switches from one tab to another.
### Generic Display Panel
A generic panel is always attached below the Tabs Ribbon Bar. It holds a few functions to view the model:

- The search bar
- The display functions
- The box functions
- The zoom functions
- The colour functions

### Specific Function Panel

One specific function panel per tab, giving access to that tab's own functions. The content of each tab's panel will be describe in each function. 

### Panels
Panels are blocks that can be reorganised within the tab. However, a few placement rules are kept to stay oriented in the project tabs.

Properties is always top right.

Current panels:

- Properties
- Models
- Spatial Hierarchy

## Open Subject

- Content of the specific function panel for each tab.
