# BonsaiViewer10 - BCF Panel


## Introduction

A BCF panel lets the user load, browse, edit and save BCF files on the models the viewer already has open. The panel implements the features of the BuildingSMART **BCF-XML 3.0** specification, as documented in the [BCF-XML release\_3\_0 documentation](https://github.com/BuildingSMART/BCF-XML/tree/release_3_0/Documentation).

The author of the topics and comments is filled automatically from the **Settings**.



The panel **reads and writes** BCF files in versions **2.1 and 3.0** of BCF-XML.

## Specification

### BCF Panel

The panel offers the following functions:

- **Load a BCF file**: opens the file explorer to find the file.
- **Save a BCF file**: opens the file explorer to choose the location of the file, as well as the version of the file (2.1 or 3.0, as the file type).
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

## Implementation

**Library.** IfcOpenShell's `bcf` library (`src/bcf`) is Python: it is ported to C++ in `ports/bcf`, as IfcTester was (`ports/ifctester`).
- One version-agnostic model (`Bcf.h`, as `bcf.agnostic`) read from and written to BCF-XML **2.1** and **3.0** (`BcfXml`); XML with QXmlStreamReader / QXmlStreamWriter, ZIP with Qt's QZipReader / QZipWriter (`Zip.cpp`, the only use of the private API; QtCore since Qt 6.5, QtGui before).
- The files the model does not describe (BIM snippets, header reference files, bitmaps, 2.1 internal documents...) are kept and written back. Dates keep their xs:dateTime text.
- Checked on the library's test files (`MaximumInformation.bcf` 2.1, `Document reference internal.bcf` 3.0): saved in their own version they read back identical; the written XML validates against the BCF XSDs, and the Python library reads the files.
- Version conversion: 2.1 → 3.0, a 2.1 internal document path becomes a URL, a topic without type / status gets "Issue" / "Open" (required in 3.0); 3.0 → 2.1, an internal document becomes `../documents/<guid>` (the file stays in the archive).

**Panel.** The BCF panel replaces the former "Issues" panel (Window > Analyze > BCF, right side). Toolbar: Load, Save, New Topic; the search bar; the display mode (Table, Thumbnails, Tree).
- Save always asks the location and the version (file type "BCF 3.0" or "BCF 2.1").
- Table columns: Title, Status, Type, Priority, Assigned To, Modified, Comments (sortable). Tree: each topic with its related topics under it. Thumbnails: the image of each topic's main viewpoint (lowest index).
- A deleted topic stays listed, struck out, with **Restore the Topic** in its menu, until the next Save.

**Author.** Settings > BCF > Author (e-mail). Asked once if empty when a topic or a comment is created; written as creation / modification author.

**Viewpoints** (`services/BcfViewpoints`): the active view ↔ a BCF viewpoint.
- Captured: camera (perspective or orthogonal; view point, direction, up vector, field of view or view-to-world scale, aspect ratio), selection, visibility (the shorter list: hidden elements with DefaultVisibility true, else the visible ones), the colour each element shows, the section planes as clipping planes, view setup hints (spaces and openings shown, from the Display menu).
- Applied: camera (the viewer has no roll: the up vector is not applied), visibility, colours (the forced colours of activated filters stay), selection, clipping planes.
- Components are matched by GlobalId over every loaded model. Coordinates are the model's: the first loaded model's model-to-viewer matrix converts both ways.
- The image is the next frame of the 3D view (PNG).

**Topic page** (non-modal, one window per topic): the image of the topic (Go to the View, Edit the Image…), every field (Apply / OK; the values of the BCF extensions are proposed), document references (double click opens: web link, a file of the archive, a file relative to the BCF file, an absolute path; Add a Link…, Add a File… stores the file in the BCF), related topics both ways (double click opens the other topic; Add…, Remove), comments (image + author, date, text).
- **Add the Comment**: the first comment captures the active view and its image, the next ones only their text. **Add with the View** captures it anyway.
- A comment's right click adds **Edit the Text…** and **Delete the Comment** to the specified functions. **Refresh the Data** replaces the comment's viewpoint data (camera, selection, visibility, colours) with the active view, and keeps its image.
- **Selection…, Visibility…, Colours…**: the element lists of the selected comment's viewpoint (else the topic's), in their own window (GlobalId, Name, Class): Remove, Add the Selected Elements (from the viewer), Change the Colour…; Visibility also sets the default visibility.

**Edit the image**: Paint on Windows and Preview on macOS (the image comes back when the editor closes); elsewhere the default application (the image comes back each time the file is saved). The image goes through a temporary file.

## Open Subjects

- The viewer's camera has no roll: a BCF up vector that is not vertical is not applied.
- Several models with different coordinate transformations: the first loaded model's transformation is used for the whole viewpoint.
- BIM snippets and lines / bitmaps of a viewpoint are kept in the file but not shown.