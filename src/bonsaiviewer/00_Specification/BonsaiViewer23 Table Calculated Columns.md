<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer23 - Table Calculated Columns

**Status: idea, not finished.** This specification is the home for the
evolution of the calculated columns of the Table (BonsaiViewer07). The first
version — strictly arithmetic — is specified in BonsaiViewer07 and is not
repeated here.

## Introduction

The calculated columns of the v1 compute algebra: numbers in, numbers out.
The next step is conditional logic: formulas that compare values and return
one result or another, so a table can classify, flag and dispatch, not only
sum and multiply.

## Conditional logic (draft idea)

- **IF-like function** — compare a column value with a value or with another
  column, and return one of two results (number or text).
- **Comparisons** — `=`, `<`, `>`, `<=`, `>=`, `<>` between columns and
  constant values.
- **Candidate uses**:
    - a status column: "Planned" / "Late", comparing a date column with a
      reference date;
    - a price column applied only to the rows matching a condition
      (category, level...);
    - a dispatch column: distribute elements in groups by threshold
      (height < 3 m → "Low", else "High").

## Open Subject

Everything: the exact function list (IF only, or IFS / SWITCH / AND / OR),
the syntax, the error model (text in a numeric comparison), the interaction
with grouping (can a conditional column be a grouping key?), and the
recalculation rules when a referenced column changes.
