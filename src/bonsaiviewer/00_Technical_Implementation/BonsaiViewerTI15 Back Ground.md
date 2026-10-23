<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewerTI15 - Back Ground

Spec source: `../00_Specification/BonsaiViewer15 Back Ground.md`

## What exists today

Solid colour background from the theme setting "Viewport Background",
applied live on theme change.

## Building blocks used

- ViewerSettings — new global setting `BackgroundMode` (+ colours), not
  per project.
- Settings dialog entries.

## New code

- **Solid** — already done; becomes mode 1 of the enum.
- **Vertical gradient** (mode 2): two user-chosen colours; implement as a
  background gradient in the render pass (fullscreen quad behind the scene
  or wgpu clear-to-texture), top-to-bottom interpolation in screen space.
  Settings: two colour pickers.
- **Procedural sky** (mode 3): Nishita model. The spec itself notes it is
  hard for a limited use and suggests going back to Blender for specific
  backgrounds. Recommendation: park mode 3; ship modes 1-2. If it comes
  later, it slots in as a third background provider behind the same enum.

## Key flows

Settings change → ViewerSettings notifies → viewport rebuilds its background
provider (solid / gradient) → no scene reload.

## Technical questions

- Gradient in screen space (fixed to the view) or world space (fixed to the
  model orientation)? Spec says "from the top of the view to the bottom":
  screen space.
