<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI09 - Clash Management

Spec source: `../00_Specification/BonsaiViewer09 Clash Management.md`
Engine source: `IfcClash_Plus/engine` (C++, Apache-2.0), the `_triang` rules
Rule model source: `IfcClash_Plus/ifcclash_plus` (Python: `RuleClass.py`,
`booleanrule.py`, `pipeline.py`, `orchestrator.py`, `serialization.py`)
UI source: branch `IfcViewer_Clash` (`src/bonsaiviewer/modules/rule/`)

## Decisions (2026-11-10)

| Subject | Decision |
|---|---|
| Rules | The 26 `_triang` rules of the IfcClash_Plus C++ engine (triangles only, the fastest); shown without the `_triang` suffix. |
| Execution | **Engine linked into the viewer**, no Python at runtime. The orchestration (folders, activation, cascades, exceptions, Must, criticity, actor, grouping) is ported to C++. |
| Engine code | **Copied** into `ports/ifcclash_engine/` (the parts the `_triang` rules need), adapted to the viewer build (system OCCT, in-tree IfcParse / IfcGeom). Changes to the copy are marked `// BonsaiViewer:`. |
| UI | The UI of branch `IfcViewer_Clash` (GeometryRule and Clash Filter workspaces, Rule Library drag and drop, rule tree with slots, Rule Config, Result, Result Info), ported to the current shell (TI21 workspaces, B04 colours, B01 selection), with the spec 09 additions. |
| Scope | Everything of spec 09 at once, **except BCF** (waits for the BCF work to come to the main branch). |
| Rule file | The JSON of `serialization.py`, round-trip with the Python CLI. Rule types are the base names (`Above`), the viewer runs `Above_triang`. **No model path** is saved (`list_ifc_path` left out): a rule file runs on any set of models (2026-11-11). |
| Result geometry | The results are shown as they are (colours, visibility). The points, zones, OBB, axes, faces and rays of the selected result are drawn only on demand, one toggle each in the Clash Filter ribbon (group RESULT GEOMETRY), all off by default (2026-11-11). |

## Architecture

```
modules/rule/            UI (branch IfcViewer_Clash, ported)
  RuleLibraryPanel       families from the engine catalogue, drag sources
  RulePanel              toolbar (Save, Load, Create Rule, Create Folder, Run)
                         + tree: file / folders (check box) / rules (type
                         badge, fed-by / feeds badge) / slots
  RuleConfigPanel        parameters of the selected node
  RuleEditDialog         title, description, state, parameters; source /
                         target / context (IDS selection editor, or a rule
                         of the tree, editable in its own window: cascade);
                         exceptions, Must, criticity, actor, grouping;
                         folder activation (double click on a node)
  ResultPanel            one row per result, grouping, review status
  ResultInfoPanel        measure, info blocks
  ClashView              controller: runs, colours, selection, overlays
services/clash/          rule model and orchestration (port of the Python)
  RuleModel              RuleFile, RuleFolder, RuleCheck, Select (facet /
                         rule), BooleanRule (leaf, and, or, not),
                         AbsOrRelCheck, results, groups
  RuleJson               serialization.py format, both ways
  Orchestrator           run_rule (selections -> engine -> pipeline),
                         folders and activation, SelectRule cascades
  Pipeline               Exception -> Criticity -> Actor -> (Grouping | Must)
                         -> Select
  EngineSession          one engine Session for the loaded models, run in a
                         worker thread
ports/ifcclash_engine/   the C++ engine (copy)
```

### Engine session

- One `ifcclash::Session` for the set of loaded models that have an IFC on
  disk (path + mtime), created on the first run, dropped when the models
  change. The engine opens its own IFC files (`IfcSource`): a run works on
  files of its own, in a worker thread, apart from the panels that read
  the viewer's files on the GUI thread.
- `triangle_source = "auto"`: the `.ifcview` the viewer writes next to the
  IFC (`foo.ifc` -> `foo.ifcview`, same format, the engine reader is a copy
  of the viewer's) when it holds the element, else the IFC triangulation.
- Selections (IDS facets) run on the engine's files with the viewer's
  IfcTester port (`ports/ifctester`): the engine receives STEP ids.
- Results come back as `(file_index, entity_id)`: same files, same STEP ids
  as the viewer's models; mapped to object ids by the ElementRegistry.
- Models without an IFC on disk (`.ifcview` only, cloud stream) are left
  out of the run, with a message.

### Coordinates

The engine works in IFC world coordinates (metres). The overlays (points,
OBBs, faces, axes) are moved into the viewer's space with the model's
federation matrix (FederatedFalseOrigin · ModelTransformation ·
CoordinateOperation), as the instances are.

### Orchestration (port of the Python)

- RuleFile / RuleFolder run in tree order; an inactive folder (check box)
  is skipped; a folder activation rule (SelectFacet: non-empty; BooleanRule:
  its rules run first, then the tree is evaluated) conditions its content.
- SelectRule: the referenced rule runs in state `Select`; its selection
  (`element_to_pass` source / target, `element_state_to_pass` passing /
  complement) feeds the slot.
- Exceptions (two-object rules only): `Same_IfcSystem`, or a boolean tree
  (BooleanLeaf modes `have_result`, `have_no_result`, `have_more`,
  `have_less`, `..._or_equals`, `equals`; And / Or / Not). Spec 09 timing:
  **after** the run (the result is invalidated, status false) or
  **before** (the pair is not computed); a leaf is evaluated on all the
  pairs in one engine call (`Session::evaluate_pairs`).
- Criticity / actor: SelectFacet + label, appended to the results whose
  source or target is selected.
- Grouping (ENTITY, SOURCE, TARGET, Property, Attribute) or Must
  (`Absolute_Number`, `Absolute_Quantity`, `Relative_Number`,
  `Relative_Quantity`, operation with SOURCE / TARGET keywords): exclusive,
  as in Python. The operation is evaluated by a small arithmetic and
  comparison parser (no `eval`).

## Phases

1. Engine copy builds in the viewer; one `_triang` rule runs on two loaded
   models (test harness).
2. Rule model, JSON round-trip, orchestrator and pipeline (test harness on
   the Duplex models).
3. UI port: workspaces, library, tree, config, results, colours, selection,
   overlays; Save / Load; Run per rule / folder / selection.
4. Editing window: exceptions, Must, criticity, actor, grouping, activation.
5. BCF (after the BCF work is on the main branch).

## Implementation status (2026-11-10)

Phases 1 to 4 are done; phase 5 (BCF) waits for the BCF work.

| Part | Files | Checked |
|---|---|---|
| Engine copy | `ports/ifcclash_engine/` (CMake target `ifcclash_engine`) | builds on the system OCCT 7.6; OBB_Above_triang gives the 8 pairs of the Python engine on the Duplex, 0 IFC conversion (all triangles from the `.ifcview`) |
| Rule model, JSON | `services/clash/RuleModel.h`, `RuleJson.*`, `RuleCatalogue.*` | JSON round trip identical; a file saved by the viewer loads in the Python CLI (`load_from_json`) with its folders, cascade, exception, Must and activation |
| Orchestration | `services/clash/Orchestrator.*` | folders (check box, activation by facets / rule tree), cascades, exceptions (relation, boolean tree, before / after), criticity, actor, grouping (ENTITY, SOURCE, TARGET, property, attribute, grouping set), Must |
| UI | `modules/rule/` (RuleDocument, RuleTree, RulePanels, Editors, RuleEditDialog, Results, ClashView) | the real MainWindow offscreen: a model, a rule file, a run (141 results, mapped to the viewer's objects); the Edit window switches a slot to a cascade and the run follows it |
| Edit window | `RuleEditDialog.*` | tabs Rule (title, description, state, parameters), Selection (one tab per slot: the IDS selection editor of the filters / IDS panel with its preview, or a rule of the tree chosen in a list, with "Edit…" opening that rule's own Edit window), Exception, Must, Grouping, Criticity, Actor; offscreen: a slot set by the IDS editor and another by a cascade, the cascaded rule renamed from its own window, the run follows (4 results) |
| Slot copy / paste | `SlotClipboard.*`, right-click menus of the Rule tree slots and of the Edit window's slot tabs | the clipboard holds the slot as rule-file JSON, plus IDS XML for facets (pastes into the filters, tables and IDS panel, and back); a cascade into its own rule or into a loop is refused; offscreen: source copied to target in the Edit window, then pasted into another rule's slot from the tree |
| Shell | `MainWindow.*`: tabs GeometryRule and Clash Filter (workspaces 4 and 5), panels Rule, Rule Library, Rule Config, Result, Result Info (Window > Analyze); the former empty "Clash" panel is removed |

Changes to the engine copy (marked `// BonsaiViewer:`):

- `triang/mesh/Obb.cpp`: a 0-based point array (the optimal `Bnd_OBB::ReBuild`
  of OCCT 7.6 raises `Standard_OutOfRange` on a 1-based one);
- `geometry/Geos.cpp`: GEOS optional (`IFCCLASH_WITH_GEOS`); without it the
  rules with polygon operations raise a ConfigError;
- `sources/IfcSource.*`, `api/Engine.h`, `api/Session.cpp`: the parsed IFC
  files are exposed for the viewer's IDS selections.

The IfcOpenShell tree plug-ins (`geometry_tree_opencascade_trianglebvh`,
`..._brep`) are built with the viewer: the clash rules load them at run time.

### Limits

- **GEOS**: `libgeos-dev` is not installed on the build machine:
  ProjectedSurface, FreeSpace and TopOrBottomSurface fail with a message
  until it is installed and the viewer rebuilt (CMake finds it).
- The engine opens its own copy of the IFC files (a parse per set of
  models, kept between runs, dropped when a model is loaded or removed).
- Models without an IFC file on disk (`.ifcview` only, cloud stream) are
  left out of a run, with a warning.
- Exception "before" removes the excepted pairs from the results: the
  engine still computes them (it has no pair filter).
- The overlays use the line slot of the viewport, shared with the measure
  tools: a measure replaces them; with all the RESULT GEOMETRY toggles off
  the clash view never touches that slot.
- Review statuses (Valid / Non Determined / Invalid) live for the session;
  Save Results writes them.
- A rule edited from a cascade ("Edit…") is written when its own window is
  accepted, even if the first window is then cancelled; a nested rule (not
  in the tree) is written with the first window.
- Not shown on screen yet: the tests run offscreen.
