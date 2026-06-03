# Shape Intersection Lab

Modern OpenGL 3.3+ core profile demo with **all rendering on shaders**, powerful extensible architecture, and a fully custom shader-powered GUI.

## Features

- **Pure shader pipeline**: No fixed-function / glBegin. VAOs, VBOs, GLSL 330 for geometry, points, 2D UI and text.
- **Extensible shape system**: 9 shapes (Sphere, Cone, Box, Cylinder, Torus, 5 Platonic). Add new in `shapes.cpp` + enum in one place.
- **Accurate CPU + GPU-ready SDF**: Plane-based for polyhedra + analytic for quadrics. Used both for CPU markers and prepared for shader inside-tests.
- **Intersection visualization**:
  - Live surface sampling with magenta/yellow markers (points rendered via shader + gl_Point).
  - Per-shape shader coloring (phong + rim).
  - Ready for per-fragment "is inside other" highlighting (data structures passed, can be extended in frag).
- **Powerful custom GUI (0 deps, all on shaders)**:
  - Dark themed panels, buttons, sliders (drag to edit), checkboxes, shape grid.
  - GDI-rendered font atlas uploaded to texture → drawn with textured quads in text shader.
  - Presets, live stats (overlap estimate), toggles.
- **Rich interaction**:
  - LMB drag: orbit camera.
  - Ctrl+LMB: move Shape A.
  - Shift+LMB: move Shape B.
  - Alt+LMB: rotate both.
  - Wheel: zoom.
  - Keys for viz, pause, reset, quick pairs, F5 shader reload (embedded).
- **Animation + controls**: Continuous rotation + gentle B movement. Pause, speed, scale, camera distance live.

## Architecture (designed for extension)

```
math3d.h          - Vec3/Mat4, transforms, OrbitCamera
gl_core.h/.cpp    - Modern context (wgl attribs 3.3 core), function loader, Mesh VAO helpers, shader compile
shapes.h/.cpp     - ShapeType + registry, mesh generators (unit), GPUData (planes + extra for analytic), 
                    CPU IsInsideLocal, Scene with A/B focus + presets
renderer.h/.cpp   - 4 shader programs (shape, points, ui rects, text), DrawShape (with other-test wiring), 
                    intersection sampling, 2D immediate drawing primitives
gui.h/.cpp        - Immediate widget system (Button/Slider/Label/Panel/ShapeGrid/PresetBar), 
                    GDI font atlas creation
main.cpp          - Win32 + timer, input routing (manipulate objects), update/render loop, HUD
```

**Adding a new shape** (example "Capsule"):

1. Add `SHAPE_CAPSULE` to enum in shapes.h (before COUNT).
2. Fill `g_shapeInfos`.
3. Implement `GenCapsule(...)` (create verts/normals + fill GPUData with type + params).
4. Add case in `GenerateUnitMesh` and in `IsPointInsideLocal`.
5. (Optional) extend the analytic test in `kFragShape` GLSL.
6. Done — appears in grid, works with intersection, etc.

## Controls

| Input              | Action                              |
|--------------------|-------------------------------------|
| LMB drag           | Orbit camera                        |
| Ctrl + LMB drag    | Move Shape A                        |
| Shift + LMB drag   | Move Shape B                        |
| Alt + LMB drag     | Rotate A + B                        |
| Wheel              | Zoom                                |
| Space              | Pause / resume animation            |
| R                  | Reset camera                        |
| T                  | Toggle object auto-rotation         |
| P                  | Toggle intersection point markers   |
| W                  | Toggle wireframe                    |
| H                  | Toggle intersection highlight mode  |
| A                  | Toggle axes                         |
| 1..5               | Quick change B (pair with A)        |
| F5                 | Reload shaders (from embedded)      |
| Esc                | Quit                                |

GUI (left panel + right stats) is live — click shape buttons, drag sliders.

## Build & Run

- Visual Studio 2019+ (toolset v142/v143)
- Win32 / x64
- Requires decent GPU driver with OpenGL 3.3+ core support.
- No external libraries (pure Win32 + opengl32).

Open `t1.sln`, build Debug, run.

Shaders are currently embedded as raw string literals in `renderer.cpp` for guaranteed run. Easy to externalize to `shaders/*.glsl` + hot reload.

## Future / Powerful Extensions (easy because of arch)

- Compute shader for thousands of intersection samples / particles.
- Full per-fragment SDF intersection tint (pass other model inverse + type to frag).
- Raymarched debug inset (2D slice or small 3D volume).
- More shapes, nonuniform scales, materials.
- Scene with N>2 shapes, any-pair intersection viz.
- Export OBJ of current meshes or intersection curves.

This is a solid foundation for a professional tool or research viz for constructive solid geometry / implicit surfaces.

## Original

The project started from a legacy fixed-pipeline Win32+glBegin demo (SphereCone3D.cpp + old 2D GDI). Completely reworked into the modern architecture above.
