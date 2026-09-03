# Hello Triangle

The smallest graphics sample and the recommended first look at Elemental's runtime API.

It creates a window, graphics device, command queue and swap chain, then renders a rotating triangle with a mesh shader.

## What it demonstrates

- application and window lifetime;
- graphics-device creation;
- a graphics command queue and command list;
- swap-chain rendering and presentation;
- shader-library loading and graphics-pipeline creation;
- push constants;
- a render pass;
- mesh-shader dispatch.

The triangle is generated entirely inside the shader, so there is no vertex buffer or asset loading to distract from the basic GPU execution path.

## Frame flow

```text
get command list
    -> begin render pass
    -> bind graphics pipeline
    -> push aspect ratio + rotation
    -> dispatch mesh shader
    -> end render pass
    -> commit + execute
    -> present
```

`Data/Triangle.hlsl` emits three vertices and one triangle from `MeshMain`, then `PixelMain` outputs the interpolated vertex color.

## Key files

- [`main.c`](main.c) — application setup and per-frame command recording.
- [`Data/Triangle.hlsl`](Data/Triangle.hlsl) — procedural triangle mesh and pixel shaders.

## Build

From the repository root:

```bash
cmake --preset default
cmake --build --preset default --target HelloTriangle
```

The sample also accepts `--vulkan`, primarily to select Vulkan instead of the default Direct3D 12 backend on Windows.

[Back to samples](../../README.md)
