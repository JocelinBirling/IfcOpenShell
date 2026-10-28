# BonsaiViewer10 - BCF Panel


## Introduction

A BCF panel lets the user load, browse, edit and save BCF files on the models the viewer already has open. The panel implements the features of the BuildingSMART **BCF-XML 3.0** specification, as documented in the [BCF-XML release\_3\_0 documentation](https://github.com/BuildingSMART/BCF-XML/tree/release_3_0/Documentation).

The author of the topics and comments is filled automatically from the **Settings**.



The panel **reads and writes** BCF files in versions **2.1 and 3.0** of BCF-XML.

## Specification

### BCF Panel

The panel offers the following functions:

- **Load a BCF file**: opens the file explorer to find the file.
- **Save a BCF file**: opens the file explorer to choose the location of the file, as well as the version of the file (2.1 or 3.0).
- **New topic**: a button in the toolbar of the panel and an item of its context menu; the active view is captured at creation (see *Component*).
- a **search bar**, to search the topics:
  - by default, the search bar only searches the **names of the topics**;
  - typing the **`-fs` prefix** in the search bar switches it to a **full search**: the search then covers **all the properties** of the BCF topic;
- a choice of **display modes** for the topics:
  - **Table**: one topic per row;
  - **Thumbnails**;
  - **Tree**: related topics (the topics linked to each topic are shown under it).

### Topic actions

A right click on a topic offers:

- Zoom on the objects
- Isolate the objects
- Modify a topic
- Delete a topic: the topic is **marked as deleted** and removed from the BCF file **at the next Save**.

A double click on a topic zooms in on the objects and isolates them.

**Modify a topic** opens a page that presents the whole topic, with its comments.

### Topic page

The page shows all the data of the topic, and all of it can be modified.

**Adding a comment**: the first comment of a topic captures the **image and viewpoint of the active view** together with its text. When the topic already has comments, **only the text** is filled in; a dedicated button updates the viewpoint and the image when needed, to avoid multiplying unnecessary screenshots.

Each comment shows its **image** and its **text**.

A right click on a comment offers:

- Isolate the objects
- Zoom on the objects
- Retake an image: captures the current view (image + viewpoint) and replaces the image and the viewpoint of the comment
- Refresh the data

A double click on a comment moves the camera to the **viewpoint**, and applies the **selection**, the **colours** and the **visibility** of the topic.

The list of the **elements** is also available, in separate windows, for:

- the selection
- the visibility
- the colours

These lists can be modified: objects can be removed or added, and the colour can be changed.

### Document reference
**Document references** (DocumentReference) make it easy to open a document, either through an HTTP link or a Windows link. The link must be read and opened as follows:

- if the link starts with **HTTP**, the default browser is launched;
- otherwise it is a physical link:
  - if it is a **relative** link, it is resolved against the location of the BCF file;
  - if it is an **absolute** link, the file is opened directly.

Filter rule sets are saved as described in BonsaiViewer06 (Save and share).


### Related topics

Two tables:

- the topics **related by** this topic;
- the topics that **relate** this topic.

Clicking a topic navigates from topic to topic.

### Component

When a BCF topic is created, all the information of the **current active view** is captured at the moment of its creation (camera viewpoint, selection, visibility, colours, components).




### Modify the image of the topic

**Modify image from BCF** allows the image of the topic to be modified with the **default image editor of the system**, to add text or information: Paint on Windows the equivalent application on macOS and linux. The image is loaded into the editor **directly, without an intermediate save**. When the editor is closed, the modified file is **retrieved directly** and stored back into the BCF.

## Open Subjects

- How the panel is opened: ribbon group and button, tabbing next to an existing panel?