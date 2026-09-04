# Hello Compute

This sample introduces general-purpose GPU compute by rendering an interactive Julia fractal into a texture and then displaying that texture through the graphics pipeline.

It is also the first sample where resource descriptors and barriers become an important part of the frame.

## What it demonstrates

- creating a GPU heap and texture resource;
- creating separate bindless read and write descriptors for the same texture;
- compiling and dispatching a compute pipeline;
- writing an `RWTexture2D` through `ResourceDescriptorHeap`;
- transitioning the texture from compute-write access to graphics-read access;
- displaying a compute result with a fullscreen graphics pass;
- recreating GPU resources when the swap chain changes size.

## Frame flow

```text
bind compute pipeline
    -> barrier for texture write
    -> dispatch 16x16 compute groups
    -> barrier for texture read
    -> begin render pass
    -> draw fullscreen mesh
    -> end render pass
    -> commit + execute
    -> present
```

`Data/Fractal.hlsl` currently renders a Julia set. A Mandelbrot implementation is also present in the shader as an alternate experiment.

## Controls

| Input | Action |
|:--|:--|
| `W` / `A` / `S` / `D` | Move around the fractal |
| `Q` / `E` | Rotate |
| `Z` / `X` | Zoom |
| `F1` | Toggle cursor visibility |
| `Escape` | Exit |
| Mouse / touch / gamepad | Equivalent navigation controls |

## Key files

- [`main.c`](main.c) — texture allocation, descriptors, barriers, compute dispatch and presentation.
- [`Data/Fractal.hlsl`](Data/Fractal.hlsl) — 16x16 compute shader that generates the fractal.
- [`Data/Tonemap.hlsl`](Data/Tonemap.hlsl) — fullscreen pass used to display the generated texture.

## Build

```bash
cmake --build --preset default --target HelloCompute
```

The sample also accepts `--vulkan`, primarily to select Vulkan instead of the default Direct3D 12 backend on Windows.

[Back to samples](../../README.md)
