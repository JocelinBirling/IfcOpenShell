<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer26 - Property Selection

## Introduction

Many functions ask the user for a property: a filter rule (06), a table
column (07), a calculated column (23), select by property (14), the
automatic colour by property, the search bar (03). Today each one has its
own input. This specification defines **one standard way to choose a
property**, used everywhere in the tool.

First users: Filter Rule (06) and Table (07, 23). Select by Property (14)
and the Search Bar (03) follow later.

A property is always identified by two names: the **property set** (Pset,
or quantity set Qto) and the **property** inside it.

## Specification

### Source of the proposals

- The proposals come from the property sets and quantity sets found in
  **all the loaded models** (the whole federation, not only the active
  model).
- The proposals are **not** limited by the context of the function: a
  filter layer on IfcWall still proposes every property set of the models.
- Free typing stays possible: a property absent from the models can still
  be entered (for example, to check that it is missing).

### The property field

The property field is made of two inputs side by side: **Pset** and
**Property**, plus a **Browse** button.

**Pset input**
- While typing, a completion list proposes the matching property sets
  (the typed text can be **anywhere** in the name, case ignored).
- The list is sorted by number of elements, largest first.
- Choosing a property set limits the proposals of the Property input to
  that set.

**Property input**
- While typing, a completion list proposes the matching properties
  **across all property sets** (text anywhere in the name, case ignored),
  shown with the property name, its property set and a few of its values:
  `FireRating — Pset_WallCommon — REI60, REI120`.
- Choosing a proposal fills **both** inputs: the property and its property
  set.
- A property that exists in several property sets appears once per set.
- When the Pset input is already filled, **only the properties of that
  set** are proposed; to search in another set, the user clears the Pset
  input first.

**Browse button**
- Opens the property picker, laid out like the Properties panel: one group
  per property set, one row per property with its element count and a few
  of its values.
- A search field filters property sets, properties and values.
- Double-clicking a property fills both inputs.

### IDS contexts

In the functions based on IDS (filter rules 06, IDS 24), each input also
offers the IDS restriction methods: Value (default), List, Pattern, Length
(and Range for values). The completions above apply to the Value method
and to the additions of the List method.

### Value of the property

When the function also asks for a value (filter rules, select by
property), the values found in the models for the chosen property are
proposed the same way, with their element counts.

## Decisions

- Source: all loaded models.
- Matching: "contains", case ignored.
- Completion entries: property name, property set, a few values.
- Pset already filled: proposals stay inside that set.
- No context limitation: every property set of the models is proposed.
- The property sets are read from the application's property index
  (B05 IfcDataAccess / PropertyIndex), not by reopening the IFC files.

## Open Subject
