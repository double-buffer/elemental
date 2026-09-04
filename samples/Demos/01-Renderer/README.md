# Renderer Demo

This is Elemental's main renderer laboratory and stress test.

Unlike the progressive `Elemental/*` samples, Renderer is intentionally not minimal. It combines several systems to expose real API friction, synchronization problems, resource-lifetime questions and performance costs while the library evolves.

## What it exercises

- compiled scene loading and GPU scene creation;
- meshlet rendering with mesh shaders;
- bindless buffers, textures and samplers;
- depth rendering into an HDR floating-point target;
- ray-tracing acceleration structures;
- compute-based path tracing using inline ray queries;
- resource barriers between compute, graphics and presentation work;
- path-tracing accumulation;
- tonemapping and UI composition;
- GPU timestamps for major rendering phases;
- an in-engine debug UI;
- camera-state persistence.

The default scene is `Sponza/sponza.scene`. A different compiled `.scene` file can be passed as the final command-line argument.

## Rendering flow

Renderer can switch between two main paths:

```text
raster
    -> mesh-shader scene rendering
    -> HDR render target

path tracing
    -> compute dispatch
    -> HDR render target

HDR result + debug UI
    -> barriers
    -> tonemap/composite
    -> swap chain
```

Scene GPU data and ray-tracing structures are also used to stress loading and synchronization behavior rather than hiding it behind a finished engine abstraction.

## Controls

| Input | Action |
|:--|:--|
| `W` / `A` / `S` / `D` | Move camera |
| Arrow keys | Rotate camera |
| Right mouse drag | Rotate camera |
| `Space` | Toggle raster / path tracing |
| `Enter` | Toggle path-tracing accumulation |
| `F1` | Toggle cursor visibility |

Gamepad camera controls are also available.

## Command-line options

- `--vulkan` — prefer the Vulkan backend.
- `--fullscreen` — start fullscreen.
- `--gpu-debug` — enable the graphics debug layer.
- `<path>.scene` — use another compiled scene instead of Sponza.

## Key files

- [`main.c`](main.c) — renderer setup and frame graph written explicitly as command recording.
- [`DebugUI.c`](DebugUI.c) / [`DebugUI.h`](DebugUI.h) — renderer debug overlay and statistics.
- [`ElementalArt.c`](ElementalArt.c) / [`ElementalArt.h`](ElementalArt.h) — renderer-specific drawing helpers.
- [`UnityBuild.c`](UnityBuild.c) — unity-build entry point for the demo implementation.
- [`Data`](Data) — renderer shaders.

## Build

```bash
cmake --build --preset default --target Renderer
```

> [!WARNING]
> Renderer is active R&D code. TODO, HACK and BUG comments are expected here. Treat it as a place where Elemental is stressed and new requirements are discovered, not as a reference engine architecture.

[Back to samples](../../README.md)
