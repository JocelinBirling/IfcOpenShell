# BonsaiViewer03 - Search Bar

## Introduction

This search bar must allow simple and fast searches in the model. By nature it will not be exact or as precise as parameter filters, but it is accessible to all users. Everyone knows how to use Google.

## Specification

### Search

A search bar is placed in the left corner of the top band. It allows searching in the parameters of the BIM objects.
It must allow simple and fast searches. For more complex searches, other features are available.

A small button sits at the right of the search bar, with a little magnifier on it. Clicking on it runs the search. All the objects whose values match the query are isolated in the view.

By default, the search is case-insensitive. An upper-case and a lower-case letter are considered equivalent.
The search value does not have to be equal to the value of the parameter. Only the presence of the character string in the value is checked.

Without a keyword, the search bar looks in several parameters:
- Name
- Long Name
- Name of the BuildingElementStorey of the parent level
- Long Name of the parent level
- Materials
- Model name
- IfcClass
- PredefinedType


- **Run** — Enter or the magnifier button: isolates the view on the results; a « No results » popup leaves the view unchanged.
- **Prefixes** — For advanced use, prefixes can be added to the query. Several prefixes can be combined. This feature is intended for expert users. A cheat sheet of the commands will be shown when hovering over the search field.

#### Advanced search commands

To specify the type of search to perform.
- « -fs » for « Full Search »: search across all properties.
- « -prop » for property: search within a single property using the « Pset.name » syntax, followed by the query. For example, `-prop "Pset1.Voltage" 240V` searches only within the named property.
- « -case » for case: the search becomes case-sensitive.

To specify a subset to search in.
- « -view » for view: search only in the active view.
- « -box » for box: search only in the active box.

To specify an action other than the classic isolation.
- « -add » for add: adds the search results to the active view.
- « -rm » for remove: removes the search results from the view.

## Open Subject
