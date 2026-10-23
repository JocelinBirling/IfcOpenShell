# BonsaiViewer18 - Display Planning

**This can not be deployed, as is, and need a lot of work.** 

## Introduction
The idea is to visualise the planning of the project with the viewer.
Editing the planning is out of scope: we are in a viewer, not an IFC editor.

The planning comes from the IFC file itself: tasks, dates and task-to-object relations are read from the IFC (IfcTask, IfcWorkSchedule, IfcRelAssignsToTask...). No external planning file is imported.

The viewer is strictly read-only on the planning: display, filter and select only. No task, date or relation is modified, not even locally.

## Specification

### Planning From the IFC
- Read the work schedules and tasks of the IFC (IfcWorkSchedule, IfcTask, IfcRelNests, IfcRelAssignsToTask).
- Read the dates of each task (start, end, or milestones).
- Read the task-to-object relations from the IFC. If the IFC contains no relation, a task has no objects to show.

### 4D Simulation in the View
- A date slider on the 3D view.
- At the date T: objects whose task is finished are shown in a solid colour, objects whose task is in progress are shown in an accent colour, and objects whose task has not started are hidden or ghosted.
- The slider can be played (animation over time) and stepped.

### Gantt Panel
- A panel holds the task list as a Gantt: task bars on a time axis, nested by summary tasks.
- The panel is synchronised with the view: selecting a task selects its objects in the 3D view; selecting an object selects its task(s) in the Gantt.
- The Gantt is read-only: no moving, resizing or creating of bars.

## Out of Scope
- Establishing or editing a planning (the viewer is not an IFC editor).
- Importing an external planning file (MS Project, Primavera, CSV...).
- Any local what-if modification of dates.

## Open Subject
The visual states of the 4D simulation (colours, ghost vs hidden, finished objects) need to be defined.
How the different work schedules of one IFC are presented (one per schedule? selectable?) needs to be discussed.
