# Cloth Simulation

Real-time cloth simulation built with `C++17`, `Qt6`, and `OpenGL`.

This project implements both `PBD (Position-Based Dynamics)` and `XPBD (Extended Position-Based Dynamics)` cloth solvers, with a small editor-style UI for creating cloth objects, tweaking parameters, and inspecting the scene at runtime.

## Preview

## Highlights

- `PBD` and `XPBD` cloth solvers
- Grid cloth generation
- `.obj` mesh import for cloth initialization
- Plane and sphere collision objects
- Runtime object selection and property editing
- Qt-based `Hierarchy` and `Inspector` panels
- Fixed-timestep engine loop (`60 Hz`)

## Tech Stack

- `C++17`
- `Qt6 Widgets`
- `Qt6 OpenGLWidgets`
- `OpenGL`
- `GLM`
- `CMake`

## Controls

| Input              | Action                |
| ------------------ | --------------------- |
| Left Click         | Select object         |
| Right Click + Drag | Orbit camera          |
| Mouse Wheel        | Zoom camera           |
| `W A S D`          | Move camera           |
| Arrow Keys         | Move camera on plane  |
| `Q / E`            | Move camera down / up |
| `R`                | Reset cloth           |
| `Space`            | Toggle cloth anchors  |

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
- Reset or delete the selected cloth object

## Build

### Requirements

- `CMake >= 3.16`
- Qt6 with `Widgets` and `OpenGLWidgets`
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

## Simulation Overview

### Cloth Representation

- Cloth is stored as particles plus triangle indices
- Grid cloth uses `width`, `height`, and `spacing`
- Mesh cloth preserves the imported triangle topology

### Solvers

- `PBD`
  - Iterative projection of stretch and bending constraints
  - Uses stiffness-based control
- `XPBD`
  - Uses compliance-based constraints
  - Maintains per-constraint lambda values

### Collision

- `PlaneObject`
- `SphereObject`

Collision resolution projects particles back outside the collider during solver passes.

## Project Structure

```text
src/
├─ cloth/         # Cloth objects, solvers, rendering, interaction
├─ engine/        # Core object system, input dispatch, render pipeline
├─ io/            # Mesh loading
├─ platform/qt/   # Qt window, viewport, hierarchy, inspector
└─ main.cpp       # Application entry point
```

## Notable Files

- `src/cloth/components/PBDClothSimulationComponent.*`
- `src/cloth/components/XPBDClothSimulationComponent.*`
- `src/cloth/components/ClothSimulationComponentBase.*`
- `src/cloth/ClothObject.*`
- `src/cloth/ClothWorld.*`
- `src/engine/core/World.*`
- `src/platform/qt/ViewportWidget.*`
- `src/platform/qt/WorldHierarchyDock.*`
- `src/platform/qt/WorldInspectorDock.*`

## References

- Matthias Müller, Bruno Heidelberger, Marcus Hennix, John Ratcliff, **"Position Based Dynamics"**, VRIPhys 2006.  
  https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
- Miles Macklin, Matthias Müller, Nuttapong Chentanez, **"XPBD: Position-Based Simulation of Compliant Constrained Dynamics"**, Motion in Games 2016.  
  https://matthias-research.github.io/pages/publications/XPBD.pdf
