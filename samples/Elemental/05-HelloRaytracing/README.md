# Hello Raytracing

This sample introduces Elemental's ray-tracing resources and acceleration structures using a small Cornell Box scene.

It builds GPU scene data plus BLAS/TLAS acceleration structures, then uses **inline ray queries (`RayQuery`) inside regular shaders** rather than a traditional ray-generation / miss / closest-hit shader pipeline.

## What it demonstrates

- creating GPU scene data for ray tracing;
- building bottom-level and top-level acceleration structures;
- exposing a TLAS through a bindless descriptor;
- using `RayQuery` and `TraceRayInline` from HLSL;
- recovering triangle, barycentric, vertex and material data after a hit;
- raster and ray/path-traced rendering paths in the same application;
- accumulating path-tracing samples in a floating-point render target;
- tonemapping the result to the swap chain.

## Controls

| Input | Action |
|:--|:--|
| `W` / `A` / `S` / `D` | Move camera |
| Arrow keys | Rotate camera |
| Right mouse drag | Rotate camera |
| `Space` | Toggle raster / path tracing |
| `Enter` | Toggle path-tracing accumulation |
| `F` | Toggle scene animation |
| `1` / `2` | Decrease / increase path length |
| `F1` | Toggle cursor visibility |
| `Escape` | Exit |

Gamepad bindings are also available through the shared camera input helper.

The camera state is saved between runs in `SavedState.bin`.

## Key files

- [`main.c`](main.c) — scene setup, acceleration structures, render paths and accumulation.
- [`Data/Raytracing.hlsl`](Data/Raytracing.hlsl) — inline ray queries and path-tracing logic.
- [`Data/RenderMesh.hlsl`](Data/RenderMesh.hlsl) — raster comparison path.
- [`Data/Tonemap.hlsl`](Data/Tonemap.hlsl) — final display pass.
- [`Data/ShaderData.h`](Data/ShaderData.h) — CPU/shader shared data layout.

## Build

```bash
cmake --build --preset default --target HelloRaytracing
```

Supported sample flags include `--vulkan`, `--fullscreen` and `--gpu-debug`.

> [!NOTE]
> This is intentionally a larger sample than the previous four. Shared scene, GPU-memory and ray-tracing helpers keep the application focused on the high-level flow, but those helpers are sample infrastructure rather than additional Elemental API layers.

[Back to samples](../../README.md)
