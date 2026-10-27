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

- **Entity**: IFC class and PredefinedType. Exactly one per specification (BonsaiViewer06).
- **Property**: property set, property name, value. Any number (BonsaiViewer06, BonsaiViewer26).
- **Attribute, Classification, Material, PartOf**: part of the IDS standard, not available in the first version (BonsaiViewer06, Open Subject).

Facets can be used in applicability or in requirements. A property facet can omit its value to only check existence.

### Value methods

Same methods as BonsaiViewer06, matching the IDS restrictions:

- **Simple value**: one exact value.
- **Enumeration**: one value from a list.
- **Pattern**: an IDS regular expression (simpler than full XML regex). The panel offers "starts with", "contains", "ends with" or a free expression.
- **Bounds**: minimum and maximum, each inclusive or exclusive (numeric).
- **Length**: exact, minimum, maximum or both.

### Panel functions

- **New**, **Open**, **Save**, **Save As** for `.ids` files.
- **Add Specification**, **Remove Specification**, **Modify**. A specification has a description, an applicability and requirements.
- **Run**: checks the loaded models against the IDS. Results are listed per specification: elements passing and failing.
- Selecting an element in the results isolates it in the viewer.
- **Export report**: planned, format to be decided.

### Relation with other panels

- The property selection uses BonsaiViewer26 (Property Selection).
- The Filter panel (BonsaiViewer06) can import an IDS specification as a layer. The IDS panel can run the same specification.

## Open Subject

- Color Property By IDS: colour elements by the properties checked by an IDS.
- Filter Property By IDS: in the Properties panel, show only the properties asked for in the IDS.
- Attribute, classification, material and partOf facets (to add after the first version).
- Report format (CSV, HTML, BCF link).
- The facet guides and the developer guide of the IDS manual were not reviewed in detail for this spec. To check before implementation.



@Todo How to ship the project if it use IfcTester in python, with a IfcTester 