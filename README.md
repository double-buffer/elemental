# Elemental

![License](https://img.shields.io/github/license/double-buffer/elemental.svg)
![GitHub Repo stars](https://img.shields.io/github/stars/double-buffer/elemental?style=flat)
[![GitHub Release Downloads](https://img.shields.io/github/downloads/double-buffer/elemental/total)](https://github.com/double-buffer/elemental/releases)
![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/double-buffer/elemental/build-ci.yml?branch=main)

**Elemental is a low-level, cross-platform library for modern GPU programming.**

It provides a small C API over Direct3D 12, Vulkan and Metal while deliberately targeting modern GPU capabilities instead of carrying legacy hardware and API constraints.

> **Elemental does not try to simplify GPU programming. It tries to simplify access to GPU programming.**

Meaningful GPU concepts such as resources, memory, queues, command lists, pipelines, descriptors, barriers, compute, mesh shaders and ray tracing remain visible. Platform boilerplate and backend-specific ceremony stay behind the library.

> [!WARNING]
> Elemental 1.0 is under active development. Breaking API changes are expected before the first stable release.

## Design principles

- **Modern GPUs first.** Elemental prefers a clean model built around recent hardware instead of adding legacy fallbacks that permanently complicate the API.
- **Keep meaningful GPU concepts visible.** Learning how the GPU works is part of the point.
- **Hide accidental complexity.** Window-system plumbing and backend-specific ceremony should not leak into application code when they do not represent useful GPU semantics.
- **Small C API.** The public API uses opaque handles, plain structures, spans and explicit functions.
- **Not the lowest common denominator.** Direct3D 12, Vulkan and Metal do not always expose identical capabilities, and Elemental does not require every backend to veto modern ideas supported elsewhere.

## What can you build with it?

Elemental is intended for people who want to directly experiment with modern GPU hardware.

Typical uses include:

- custom renderers and game engines;
- GPU-driven rendering and mesh-shader experiments;
- rasterization, path tracing and ray tracing;
- simulations and general-purpose compute;
- small tensor or neural-network experiments;
- graphics and GPU research projects.

Elemental provides the low-level building blocks rather than a renderer, scene system, ECS, ML framework or engine architecture.

## Current capabilities

Elemental currently includes:

- cross-platform application lifetime and windows;
- keyboard, mouse, gamepad and touch input;
- graphics and compute command queues;
- command lists and swap chains;
- GPU heaps and resources;
- bindless resource access;
- synchronization and resource barriers;
- graphics and compute pipelines;
- render passes;
- compute shaders;
- mesh shaders;
- ray tracing;
- HLSL shader compilation tools.

## Platforms

| Platform | GPU backend |
|:--|:--|
| Windows | Direct3D 12, Vulkan |
| Linux | Vulkan / Wayland |
| macOS | Metal |
| iOS | Metal |

Feature coverage is still evolving while Elemental 1.0 is under development.

## Getting started

Prebuilt development packages are available from [GitHub Releases](https://github.com/double-buffer/elemental/releases).

To build Elemental from source:

```bash
git clone --recurse-submodules https://github.com/double-buffer/elemental.git
cd elemental

cmake --preset default
cmake --build --preset default
```

The progressive samples are the best introduction to the API:

1. [`01-HelloTriangle`](samples/Elemental/01-HelloTriangle)
2. [`02-HelloInputs`](samples/Elemental/02-HelloInputs)
3. [`03-HelloCompute`](samples/Elemental/03-HelloCompute)
4. [`04-HelloMesh`](samples/Elemental/04-HelloMesh)
5. [`05-HelloRaytracing`](samples/Elemental/05-HelloRaytracing)

Larger experiments live in [`samples/Demos`](samples/Demos).

## Future directions

Elemental is also a research project around modern GPU programming.

Areas of interest include:

- GPU virtual memory and modern resource models;
- GPU-driven execution and Work Graphs;
- advanced ray tracing and geometry processing;
- hardware-accelerated linear algebra and tensor operations;
- hardware video decoding and encoding.

These are research directions, not promises of stable APIs.

## License

Elemental is released under the [MIT License](LICENSE).
