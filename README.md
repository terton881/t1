# Intersection Drafting Lab

A lightweight **Win32 + OpenGL 3.3** prototype for engineering exploration of **two 3D solids**: interactive placement, intersection sampling, sorted intersection curve in 3D, and simple **drafting projections** (Front / Top / Right).

This is **not** an AutoCAD clone. No DWG, no CSG kernel, no NURBS — only a focused research/drafting tool built on the existing shader pipeline.

## Features (MVP)

- Two bodies **A** and **B** with 11 shape types (sphere, cone, box, platonic solids, torus, capsule, pyramid, …)
- **3D view**: Phong shading, inside-other highlight, intersection points, sorted intersection curve
- **Drawing sheet**: three orthographic views with grid, outlines, and intersection curve (yellow)
- **Split mode**: 3D + drafting sheet side by side
- **Dirty-flag intersection**: recomputed only when geometry changes (not every frame)
- Quality presets: Low (800) / Medium (2200) / High (6000) samples per shape
- Separation along **X / Y / Z** (PgUp/PgDn or +/-)

## Build & Run

- Visual Studio 2019+ (toolset v142), **Win32**
- Open `t1.sln`, build **Debug** or **Release**, run `t1.exe`
- Requires OpenGL 3.3+; dependency: `opengl32.lib` only

## Controls

| Input | Action |
|-------|--------|
| LMB drag | Orbit camera |
| Ctrl + LMB | Move shape A |
| Shift + LMB | Move shape B |
| Alt + LMB | Rotate A and B |
| Wheel | Zoom |
| X / Y / Z | Separation axis for B |
| PgUp / PgDn, +/- | Move B along separation axis |
| Space | Pause animation |
| T | Toggle auto-rotation |
| P | Toggle intersection points |
| C | Toggle 3D intersection curve |
| V | Toggle drafting projections |
| W | Wireframe |
| H | Intersection highlight |
| A | Axes |
| F5 | Reload embedded shaders |
| Esc | Quit |

GUI (left panel): mode (3D / DRAW / SPLIT), shape grids, scales, quality, presets, toggles.

## Architecture

```
main.cpp           Win32 bootstrap, WndProc → App
app.h/.cpp         Application: update, render, input, view modes
document.h/.cpp    CadDocument: objectA/B, intersection, projection sheet
intersection.h/.cpp IntersectionSolver (CPU sampling, curve sort)
projection.h/.cpp  Front/Top/Right 2D projections + ProjectionSheet
shapes.h/.cpp      Shape meshes, CPU inside tests
renderer.h/.cpp    GLSL draw only (3D, points, curve, 2D sheet)
gui.h/.cpp         Immediate-mode UI + font atlas
math3d.h           Vec2/Vec3/Mat4, OrbitCamera
gl_core.h/.cpp     WGL 3.3 context, GL loader
```

**Layering rules**

- Geometry lives in `intersection` / `document` — not in `renderer`
- `renderer` only displays `IntersectionResult` and `ProjectionSheet`
- `main.cpp` has no scene business logic

## Roadmap (not in MVP)

- DWG/DXF, true CSG, dimensions, undo/redo, file format
- External `shaders/*.glsl`, mesh caching, compute-shader sampling
- Snap/grid, richer silhouette extraction for projections

## History

Evolved from the original fixed-pipeline `SphereCone3D` demo into a shader-based Shape Intersection Lab, then refactored into this drafting-oriented architecture.