# Cloth Simulation

Real-time cloth simulation built with `C++17`, `Qt6`, and `OpenGL`.

The project includes both `PBD` and `XPBD` cloth solvers, a small editor-style runtime UI, collision objects, mesh/grid cloth creation, and interactive particle grabbing in the viewport.

## Preview
https://github.com/user-attachments/assets/9778a78b-dd9f-489e-bfe1-7441acf810fb

## Current Features

- `PBD` and `XPBD` cloth solvers
- Fixed-step simulation at `60 Hz`
- Small-step style solver loop with configurable `substeps` and `constraint_iterations`
- Grid cloth creation
- `.obj` mesh cloth import
- Grid-like mesh detection and remapping to regular cloth layout when possible
- Distance-only cloth constraints:
  - `Stretch`
  - `Shear`
  - `Bend` distance constraints
- Spatial-hash-based particle self collision
- Plane and sphere collision objects
- Demo-style plane contact friction by rolling back per-step motion
- Runtime object hierarchy and property inspector
- Interactive cloth particle grabbing with the mouse

## Controls

| Input | Action |
| --- | --- |
| Left Click | Select object |
| Left Click + Drag on cloth | Grab the nearest particle to the first hit point and drag it |
| Right Click + Drag | Orbit camera |
| Mouse Wheel | Zoom camera |
| `W A S D` | Move camera |
| Arrow Keys | Move camera |
| `Q / E` | Move camera down / up |
| `R` | Reset all cloth objects |
| `Space` | Toggle anchors for all cloth objects |

Notes:

- Inspector `Reset` and `Delete` act on the currently selected cloth.
- Mouse grabbing fixes the grabbed particle while dragging and releases it on button up.

## Editor UI

### Hierarchy

- View all world objects
- Create grid cloth
- Import mesh cloth from `.obj`
- Choose solver type: `PBD` or `XPBD`
- Spawn a sphere projectile from the current camera view

### Inspector

- Inspect the selected object
- Edit object and component properties at runtime
- Adjust cloth solver and simulation parameters
- Reset or delete the selected cloth object

## Simulation Notes

### Cloth Representation

- Cloth is stored as particles plus triangle indices.
- Grid cloth stores explicit `width`, `height`, and `spacing`.
- Imported mesh cloth preserves triangle topology.
- A small initial out-of-plane jitter is added at creation time to break perfect planar symmetry.

### Constraint Model

The solver currently uses `DistanceConstraint` only.

- For regular grid cloth, or meshes that can be inferred as a regular grid patch, the solver builds the same six distance-constraint families used by the Matthias Mueller demo:
  - vertical stretch
  - horizontal stretch
  - two shear diagonals
  - vertical bend-distance
  - horizontal bend-distance
- For arbitrary triangle meshes, the solver falls back to:
  - unique triangle edges as `Stretch`
  - opposite vertices across a shared edge as `Shear` or `Bend`, classified heuristically from rest lengths

This means grid and grid-like cloth match the demo path much more closely than arbitrary triangulated meshes.

### Solver Behavior

- `PBD` uses per-kind stiffness values:
  - `Stretch Stiffness`
  - `Shear Stiffness`
  - `Bend Stiffness`
- `XPBD` uses per-kind compliance values:
  - `Stretch Compliance`
  - `Shear Compliance`
  - `Bend Compliance`
- `substeps * constraint_iterations` determines the total number of small solver passes per frame.
- The default setup is tuned toward small-step behavior:
  - `substeps = 10`
  - `constraint_iterations = 1`

### Self Collision

- Self collision is particle-particle only.
- Broadphase uses a spatial hash.
- Candidate pairs are prepared once per frame and reused across solver passes.
- Collision filtering uses rest-state distance checks similar to the demo.
- `collision_margin` is currently used as:
  - external collision margin
  - self-collision minimum separation distance
  - spatial hash cell size

### External Collision

- `PlaneObject`
- `SphereObject`

Plane collision uses demo-style friction:

- move the particle back along its current-step displacement
- then project it back to the plane contact offset

`PlaneObject` exposes `Collision Friction` in the inspector. `1.0` is the demo-like fully sticky setting.

## Default Scene

The world starts with:

- one imported cloth mesh from `asset/test_cloth_patch.obj`
- a floor plane placed below the cloth
- a movable camera with orbit and zoom controls

## Build

### Requirements

- `CMake >= 3.16`
- Qt6 with `Widgets`, `OpenGL`, and `OpenGLWidgets`
- OpenGL-capable environment
- C++17-compatible compiler

### Build with Qt presets

If your local Qt path differs from the preset, update `QTDIR` or `CMakeUserPresets.json` first.

```powershell
cmake --preset Qt-Debug
cmake --build out/build/debug
.\out\build\debug\PBDClothSimulator.exe
```

Release build:

```powershell
cmake --preset Qt-Release
cmake --build out/build/release
.\out\build\release\PBDClothSimulator.exe
```

### Generic CMake build

```powershell
cmake -S . -B out/build/release -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"
cmake --build out/build/release
.\out\build\release\PBDClothSimulator.exe
```

### Windows / MSVC note

If you build with `MSVC + Ninja` from a plain shell, make sure the Visual Studio developer environment is loaded first.

Example:

```powershell
cmd.exe /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cmake --build "C:\path\to\Cloth-Simulation\out\build\debug"'
```

## Project Structure

```text
src/
|- cloth/         # Cloth objects, solvers, rendering, interaction
|- engine/        # Core object system, input dispatch, render pipeline
|- io/            # Mesh loading
|- platform/qt/   # Qt window, viewport, hierarchy, inspector
`- main.cpp       # Application entry point
```

## Notable Files

- `src/cloth/components/PBDClothSimulationComponent.*`
- `src/cloth/components/XPBDClothSimulationComponent.*`
- `src/cloth/components/ClothSimulationComponentBase.*`
- `src/cloth/ClothObject.*`
- `src/cloth/core/Cloth.*`
- `src/cloth/ClothWorld.*`
- `src/engine/core/World.*`
- `src/engine/objects/PlaneObject.*`
- `src/platform/qt/ViewportWidget.*`
- `src/platform/qt/WorldHierarchyDock.*`
- `src/platform/qt/WorldInspectorDock.*`

## References

- Matthias Mueller, Bruno Heidelberger, Marcus Hennix, John Ratcliff, **"Position Based Dynamics"**, VRIPhys 2006.  
  https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
- Miles Macklin, Matthias Mueller, Nuttapong Chentanez, **"XPBD: Position-Based Simulation of Compliant Constrained Dynamics"**, Motion in Games 2016.  
  https://matthias-research.github.io/pages/publications/XPBD.pdf
