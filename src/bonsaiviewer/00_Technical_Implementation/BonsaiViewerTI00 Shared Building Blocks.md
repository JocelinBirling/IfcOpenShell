<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI00 - Shared Building Blocks

Spec source: all finished specifications in `../00_Specification/`.
Purpose: catalogue the technical bricks that several specifications need. Each
brick is designed once, implemented once, in `src/bonsaiviewer/services/`, and
consumed by the features. Build them before the features that need them.

## Existing code to extend (do not duplicate)

- `SessionState` — central signals hub (selection, models, federation,
  visibility, progress, project). All bricks report through it.
- `ElementRegistry` — `object_id` to `express::base` entity lookup, per model,
  `BasicElementInfo` listing. The data backbone of brick B05.
- `modules/viewport/Commands` — current visibility commands (hide, isolate,
  show all) become thin wrappers around B02.
- `components/` — Panel, Tabs, Section, KeyValueTable, Dialog, Buttons: the UI
  vocabulary of bricks B12, B13, B15.
- `Measurement.cpp` — LengthMeasurement / AreaMeasurement pattern (onPick,
  clear, rebuildOverlay) extended by TI11 and reused for hover feedback design.
- `ViewerSettings` — persisted application settings; B14 is its JSON sibling
  for richer documents (filters, layouts, shortcuts).

## The bricks

| Id | Brick | Responsibility | First needed by |
|----|-------|----------------|-----------------|
| B01 | SelectionService | Multi-object selection as an ordered set of object ids, with operation modes (replace / add / remove) and a source tag (viewport, tree, table, search). Emits `selectionChanged(set, source)` via SessionState. | 02, 03, 04, 12, 14, 16 |
| B02 | ActiveView | The set of displayed elements of the current view. Operations: `isolate(set)`, `add(set)`, `remove(set)`, `showAll`, `hideAll`, `invert`. Visibility of an object = in active view AND not removed by B03. | 02, 03, 04, 06, 12, 14, 18 |
| B03 | CategoryOverrideFilter | Global category toggles (IfcSpace, IfcOpeningElement, IfcZone, IfcDrawing, quick categories). A disabled category is never shown, whatever the path (isolation, search, Show All). Final gate of B02. | 01, 02 |
| B04 | AppearanceService | Per-object colour and opacity overrides with priority layers (user paint > table colour > filter action > clash run), original colour preservation, reset on model reload. | 02, 06, 07, 14, 19 |
| B05 | IfcDataAccess + PropertyIndex | Uniform access to element attributes, Psets, quantities (via ElementRegistry); plus a per-model searchable index: field values (Name, Long Name, parent storey, materials, IfcClass, PredefinedType) and inverted indexes (value to ids, property to ids). Refreshed on model load / geometry ready. | 03, 06, 07, 12, 14, 16, 17 |
| B06 | QueryEngine | Parses query strings with prefixes (`-fs`, `-prop`, `-case`, `-view`, `-box`, `-add`, `-rm`), evaluates them against B05, returns an element set. Extensible into the IDS-facet selection of TI06. | 03, 06, 14 |
| B07 | EntitySetStore | Named element sets with session lifetime: the current Box and its 3 slots. Set algebra (add, remove, replace, intersect with valid ids). Purged when a model closes. | 04, then 03, 06, 07 |
| B08 | ActionPipeline | The shared executor of display actions: isolate / add to view / remove from view / colour / transparency / auto-colour by property, applied to an element set, in order. One implementation used by search, filters, select-by-property, display functions. | 02, 03, 06, 14 |
| B09 | PickingService | Viewport raycast returning entity-level results: hit object id, hit type (point / edge / face), world position, surface normal; hover highlight callbacks. | 02, 08, 11, 20 |
| B10 | CameraService | Fit view (bounds of visible elements, 200 m heuristic), perpendicular-to-normal framing, zoom presets (center / cursor / selection), orbit presets (view center / cursor), home. | 02, 08 |
| B11 | SectionPlaneManager | Multiple section planes, each with origin + normal; gizmos (drag to translate, right-click delete), global hide and suspend toggles; all cuts apply simultaneously. Generalises the single section plane of today. | 02 |
| B12 | TreePanelFramework | Reusable tree panel parameterised by a TreeDataSource: class/instance grouping, folders, drag and drop, visibility column, context menus. Conventions: click selects (B01), double-click isolates (B08). | 12, 13, 06, 07 |
| B13 | PanelManager | Registry of panel factories, per-tab dock layout, add/remove panel at runtime, generate the Window menu (TI01), user tabs with "+" (TI21). | 01, 05, 16, 21 |
| B14 | PersistenceService | JSON documents in `QStandardPaths::AppDataLocation` (filters, layouts, shortcuts, default paths) with import / export. Survives sessions. | 06, 21, 22, 08 |
| B15 | ContextMenuBuilder | Declarative context menus (items, enablement, visibility) attached to properties rows, measurements, viewport objects. | 11, 14, 20 |
| B16 | StepTextService | Raw STEP line access per entity (file retained or re-read), hashtag parsing, declaration to usages navigation, ascending order. | 16 |
| B17 | PopupService | Standard popups: information ("No results", "The objects are far away", "The active view is empty"), confirmations (delete, folder load). | 02, 03, 06, 21, 22 |

## Construction phases (dependency order)

- Phase 1 — foundation: B01, B02, B03, B05, B14, B17.
- Phase 2 — interaction: B09, B10, B15, B04.
- Phase 3 — features: B06, B07, B08, B11, B12, B16.
- Phase 4 — shell: B13 (last, it wires the panels of everything else).

## Specification to brick map

| Spec (finished) | Consumes | Produces / extends |
|-----------------|----------|--------------------|
| TI01 Menu Bar | B03, B13, B17 | — |
| TI02 Display Function | B02, B03, B04, B08, B09, B10, B11, B17 | — |
| TI03 Search Bar | B01, B02, B05, B06, B07, B08, B17 | — |
| TI04 Box | B01, B07, B08, B02 | B07 |
| TI05 Display Management | B13 | generic top panel layout |
| TI06 Filter Rule | B05, B06, B07, B08, B12, B14, B15, B17 | IdsSelectionEngine, FilterRunner |
| TI07 Table | B01, B04, B05, B07, B08, B12, B14, B15 | table engine, RelationEngine, FormulaEngine |
| TI08 Orbit and Zoom | B09, B10 | — |
| TI11 Distance | B09, B15 | DistanceMeasurement class |
| TI12 Ifc Class Tree | B01, B02, B05, B08, B12 | class tree data source |
| TI13 Unique Tree | B12, B05 | room/level assignment engine |
| TI14 Select By Property | B01, B05, B08, B15, B04 | — |
| TI15 Back Ground | ViewerSettings | background render mode |
| TI16 Ifc Text Digger | B01, B13, B16 | text panel |
| TI21 Add Panel | B13, B14 | user tabs |
| TI22a Easy Load File | B14, B17 | drop handler, path history |
| TI22b Keybindings | B14 | ShortcutRegistry |

## Not covered yet (specs not finished)

TI09 / TI10 (drafts), TI17 Cost, TI18 Planning, TI19 Drawings (marked as
not deployable as is), TI20 Geometric Info (TBD section), TI23 Table
Calculated Columns (idea), TI99 (idea). Their bricks (B05, B06, B08, B12
especially) are already prepared by this catalogue so the fiches can
follow quickly once the specs are finished.
