# BonsaiViewer24 - IDS

## Introduction

An IDS panel is added to the viewer. It creates, saves, loads and runs IDS files, following the buildingSMART IDS standard (`.ids` files, XML).

An IDS file lists **specifications**. Each specification has:

- a **description**: the human-readable reason and instructions;
- **applicability**: the elements the specification targets, built from facets;
- **requirements**: the information those elements must have, built from facets.

The IDS standard cannot check geometry, calculated values, or data outside the IFC model. Those are out of scope for this panel.

The selection logic (facets, value methods, the IfcTester engine) is the same as in BonsaiViewer06 (Filter Rule). This spec covers the file itself and the panel.

## Specification

### IDS file

File-level metadata, filled in the panel:

| Field | Content |
|-------|---------|
| title | Title of the IDS document |
| copyright | Copyright owner |
| version | Version in the form X.Y. Y: minor change (metadata, description, spelling). X: major change, where models may pass or fail differently |
| description | One or more sentences: who the IDS is for, why it exists, which projects it applies to |
| author | Author contact (email) |
| date | Publication date (YYYY-MM-DD) |
| purpose | Why the information is needed (e.g. clash detection, cost estimation, coordination) |
| milestone | Delivery milestone (e.g. Schematic Design, Construction) |

An IFC schema is set per specification: IFC2X3, IFC4 or IFC4X3_ADD2.

### Specifications

- **Applicability cardinality** (entity count):
  - Required: at least one matching element must exist, and each must meet the requirements (minOccurs 1, maxOccurs unbounded).
  - Optional: matching elements may exist; if they do, they must meet the requirements (minOccurs 0, maxOccurs unbounded).
  - Prohibited: no matching element must exist; the requirements are ignored (minOccurs 0, maxOccurs 0).
- **Requirement cardinality** (per facet):
  - Required: the property or value must exist and not be null.
  - Optional: may be absent; if present, must match the expected type and value.
  - Prohibited: must not be present.

### Facets

The six facets of IDS 1.0 are available, in the IDS panel and in the Filters and Tables (BonsaiViewer06):

- **Entity**: IFC class and PredefinedType.
- **Attribute**: attribute name and value.
- **Classification**: classification system and reference.
- **Property**: property set, property name, data type, value (BonsaiViewer26).
- **Material**: material, layer, profile or constituent name or category.
- **PartOf**: relation (aggregate, group, spatial container, nest, void / fill) and the related class and PredefinedType.

Facets can be used in applicability or in requirements. A facet can omit its value to only check existence.

**Copy / paste of facets**: facets are copied from and pasted into the IDS panel, the Filters and the Tables. The clipboard holds IDS XML: a `<facets>` element (IDS namespace) with the facet elements, as written in an `.ids` file. Pasting also accepts a lone facet element, or a whole IDS document (the facets of its first specification). Pasting a facet that the target cannot use (e.g. a second entity facet) is refused with a message.

### Value methods

Same methods as BonsaiViewer06, matching the IDS restrictions:

- **Simple value**: one exact value.
- **Enumeration**: one value from a list.
- **Pattern**: an IDS regular expression (simpler than full XML regex). The panel offers "starts with", "contains", "ends with" or a free expression.
- **Bounds**: minimum and maximum, each inclusive or exclusive (numeric).
- **Length**: exact, minimum, maximum or both.

### Panel functions

The IDS panel is at the bottom, in the tabs of the Table and the Ifc Text Digger, hidden at start-up; Window > IDS shows it.

- **New**, **Open…**, **Save**, **Save As…** for `.ids` files; the file name, the title and `*` (unsaved changes) are shown. New, Open and quitting the viewer offer to save the changes.
- **Information…**: the file metadata (title, copyright, version, description, author, date, purpose, milestone).
- **Specifications**: list on the left, **+ Add** (a new one starts with an entity facet on IfcWall), **Duplicate**, **Remove**. The selected one has four tabs:
  - **Specification**: name, identifier, description, instructions, IFC versions (IFC2X3, IFC4, IFC4X3_ADD2), applicability cardinality (required / optional / prohibited) with its explanation;
  - **Applicability** and **Requirements**: the facet editor of the filters (all six facets, copy / paste), without the one-entity rule and without preview. The property lists of the requirements follow the entity of the applicability. Requirements are disabled for a prohibited specification.
  - **Results**: see Run.
- **Run**: checks every loaded model against the IDS (C++ engine, on the open files). A specification is judged on the whole federation, on the models of its IFC versions (the others are skipped and counted): required fails when no element is applicable, prohibited when one is, and otherwise when an element fails a requirement.
- **Results**: one row per specification (✔ / ✘, applicable and failed counts), one per requirement (in words, passed and failed counts), one per failing element (model, `#id`, class, name, reason; the first 300 per requirement).
  - A click on a failing element isolates it.
  - Right-click: on a specification, isolate its applicable, passing or failing elements; on a requirement, isolate its failing elements.
  - Elements without geometry cannot be isolated; the status bar says how many.
  - **Colour the objects** (check box above the results, off by default): once checked, a double-click on a requirement isolates the applicable elements of its specification and colours them: red when they fail this requirement, green otherwise. Right-click also offers to colour a specification (passing green, failing red) or a requirement, and to remove the IDS colours; unchecking the box removes them.
  - The colours use their own layer of the appearance service (IDS result: below the user paint and the table colours, above the filter colours), so they never erase a colour set by hand.
- **Opened IDS kept exact**: the editor normalises what it shows (class and PredefinedType case, one pattern per field…). While the facets of a specification are not edited, Save and Run use them exactly as read. A specification whose facets the editor cannot show (e.g. a totalDigits restriction) is kept as read, runs, and its facets are read-only, with a note.
- **Export report**: planned, format to be decided.

### Relation with other panels

- The property selection uses BonsaiViewer26 (Property Selection).
- The Filter panel (BonsaiViewer06) can import an IDS specification as a layer. The IDS panel can run the same specification.

### IDS engine and delivery

**History.** The IDS work first ran in IfcTester, in a Python worker process (`src/ifctester/ifctester_ipc.py`, option A: script shipped next to the executable, Python installed by the user). Replaced in October 2026 by the C++ engine below: the viewer no longer runs Python, Settings > Filters (interpreter, script) is removed, and the build no longer ships the script.

**Decision (October 2026): port the IDS engine to C++ (option C).** The viewer follows the pattern of the original BonsaiViewer: it never runs Python; the logic it needs from IfcOpenShell Python is ported to C++ (`src/helpers/`, like `ifcopenshell.util.element` / `util.placement`). Done: the C++ engine replaced the worker (October 2026).

#### Native IDS engine — scope
- **Same requests** as the worker, so the Filters UI is unchanged: build (facets → IDS XML), parse (IDS XML → facets), select, preview, classes, classInfo, properties, values. IdsBridge calls the C++ engine in-process instead of the Python process.
- **IDS XML**: read and written with Qt (QXmlStreamReader / QXmlStreamWriter), IDS 1.0 namespace, the same canonical form as IfcTester (applicability = entity facet, requirements = property facets).
- **Evaluation on the IFC files the viewer already has open** (no second copy in memory), with the C++ helpers (`get_psets`, `get_predefined_type`, type inheritance).
- **Facets**: Entity (exact class in upper case, enumeration of classes for the subtypes, PredefinedType with the type's value first, USERDEFINED → ObjectType / ElementType), Property (property set and property name, simple / enumeration / pattern / range / length values, cardinality required / optional / prohibited, property sets and quantity sets, values inherited from the type).
- **Values like IfcTester**: comparison by data type (text, integer, real, boolean), reals compared with IfcTester's relative tolerance, lengths, areas, volumes… converted to SI units before the comparison, XSD patterns translated to the regular expressions of Qt.
- **Schema introspection** with the IfcOpenShell C++ schema (classes, subtypes, PredefinedType enumerations) for IFC2X3, IFC4 and IFC4X3.
- **Comparison tests**: on real models, every filter must select the same elements with the C++ engine and with IfcTester before the Python worker is removed.
- **All six facets** are ported (attribute, classification, material and partOf included).

#### Native IDS engine — the port
- The ported code is kept apart, in `src/bonsaiviewer/ports/`, one folder per Python package; every file says `PORT of <python file>` and each function quotes its Python name:
  - `ports/ifcopenshell_util/`: `Attr` (attribute access by name on the schema-agnostic API), `Element` (util.element: get_type, get_predefined_type, get_container, get_parent, get_material…), `Classification` (util.classification), `Unit` (util.unit: get_property_unit, si_type_names…).
  - `ports/ifctester/`: `PyValue` (Python value semantics: ==, str, float), `Restriction` (Restriction, cast_to_value, is_x, elementpath translate_pattern), `Facets` (the six facets: filter, __call__, to_string), `Ids` (Ids, Specification: XML read / write, validate).
- Source: `src/ifctester/ifctester/facet.py` and `ids.py` of this repository.
- **Comparison test (October 2026)**: the 334 cases of the buildingSMART IDS test suite (`Documentation/ImplementersDocumentation/TestCases`), run by IfcTester and by the port: 333 identical results (status, applicable and failed elements of every specification). The last one makes IfcTester crash (IFC2X3 extended material properties); the port gives the expected result. The 21 cases where IfcTester differs from the expected result (XSD validation, float tolerance of the newer test cases, regex OR) differ in the same way in the port. Written IDS files are valid against `ids.xsd`, and read back with the same results.
- **Known differences with IfcTester**:
  - The XML is not validated against `ids.xsd` (xmlschema is not ported); unknown elements are skipped.
  - Where IfcTester raises (assert, missing attribute, unknown unit type), the port fails the facet or skips the unit conversion.
  - `get_project_unit` does not match derived units by dimensions.
  - Failure messages quote entities as `#id=IfcClass(...)` instead of the full STEP line.
- Fix found by the test: `get_psets` (`src/helpers/pset.cpp`) now reads the property sets of an IfcContext (IfcProject) in IFC4 and later, like the Python.

#### Native IDS engine — the viewer side
- `services/IdsEngine`: the requests of the former worker (build, parse, select, preview, classes, classInfo, properties, values) on the port. `IdsBridge` keeps its asynchronous interface (callbacks from the event loop, in request order) and gets the open files from the scene loader; a path not loaded is opened by the bridge itself.
- **Comparison test (October 2026)**: 94 requests of the editor (classes, class info, properties, values, previews with entity and property facets, build / parse round trip) on generated IFC2X3 and IFC4 models: same answers as the Python worker, except the IFC4X3 classes (not built on the test bench) and two requests where IfcTester crashes (property inherited from the type with an empty occurrence property set). 30 more previews with the new facets (attribute, classification, material, part of, data type): same elements as IfcTester.
- Fix found by the test: `get_psets` (`src/helpers/pset.cpp`) no longer throws on a property set or quantity set whose list of properties is unset (`$`).

#### Implementation status (October 2026)
- Done: the port (all facets), the C++ engine behind IdsBridge, the editor of the six facets in Filters and Tables, copy / paste of facets, the IDS panel (`modules/ids`: IdsDocument, Editor, View).
- Tests of the panel (bench, offscreen): the 334 IDS files of the buildingSMART suite read, written and read back give the same results as IfcTester (333 identical, as for the port); opening each of them and clicking through every specification and facet changes nothing and marks nothing as modified.
- Fix found by the test: the pattern `.*` alone was read back as an empty "starts with" in the value editor; it is now a free expression.

**Other improvement, not retained:**

- **B. Embedded Python**: ship a portable Python with IfcOpenShell and IfcTester already installed (Windows embeddable package), so nothing has to be installed. Cost: about 200 MB more in the package, one per platform.

## Open Subject

- Color Property By IDS: colour elements by the properties checked by an IDS.
- Filter Property By IDS: in the Properties panel, show only the properties asked for in the IDS.
- Report format (CSV, HTML, BCF link).
- The facet guides and the developer guide of the IDS manual were not reviewed in detail for this spec. To check before implementation.



- `src/ifctester/ifctester_ipc.py` is no longer used by the viewer: to delete. 



We want to create subjects based on the IDS result. It will create subject in bonsaiViewer and the user can then modified them.
Create BCF per requirements
Create BCF per Specifications

By default, valid object will be displayed in green and false one in red in the BCF.