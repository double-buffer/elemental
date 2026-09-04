# Hello Mesh

Hello Mesh moves from procedural geometry to compiled mesh data and meshlets.

The sample loads a small scene, creates a depth buffer and dispatches a mesh shader using the meshlet data for the first mesh primitive. It is intentionally a focused mesh-rendering sample rather than a general scene renderer.

## What it demonstrates

- loading the sample `.scene` format;
- using mesh and meshlet data produced by the Scene Compiler sample;
- allocating GPU memory for scene data;
- creating and resizing a depth buffer;
- configuring depth testing;
- passing mesh-buffer offsets to a mesh shader;
- dispatching one mesh-shader workgroup per meshlet;
- interactive model-viewer controls.

The sample currently renders the first primitive from `kitten.scene`. The surrounding scene/GPU-memory helpers are shared sample code and are still evolving.

## Controls

| Input | Action |
|:--|:--|
| `W` / `S` | Rotate up / down |
| `A` / `D` | Rotate left / right |
| `Q` / `E` | Roll |
| `Z` / `X` | Zoom |
| `Space` | Toggle meshlet visualization |
| `F1` | Toggle cursor visibility |
| `Escape` | Exit |
| Mouse / touch / gamepad | Equivalent model-viewer controls |

## Key files

- [`main.c`](main.c) — scene loading, GPU resources, depth buffer and mesh dispatch.
- [`Data/RenderMesh.hlsl`](Data/RenderMesh.hlsl) — meshlet-based mesh and pixel shaders.
- [`../../ElementalTools/02-SceneCompiler`](../../ElementalTools/02-SceneCompiler) — creates the sample scene data consumed here.

## Build

```bash
cmake --build --preset default --target HelloMesh
```

Supported sample flags include `--vulkan`, `--fullscreen` and `--gpu-debug`.

> [!NOTE]
> This sample deliberately contains some shared helper code and temporary memory/scene-loading choices. It demonstrates the mesh-shader path; it is not intended to define a final renderer architecture.

[Back to samples](../../README.md)
