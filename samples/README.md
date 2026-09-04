# Elemental Samples

The sample tree is split into three groups with different goals:

- [`Elemental`](Elemental) contains small, progressive samples focused on the public runtime API.
- [`Demos`](Demos) contains larger experiments used to stress Elemental and explore renderer or GPU-programming ideas.
- [`ElementalTools`](ElementalTools) contains small command-line examples for offline asset and shader processing.

The runtime samples are intentionally low level. They keep resources, queues, command lists, descriptors, barriers and pipelines visible instead of hiding them behind a sample framework.

## Build

From the repository root:

```bash
cmake --preset default
cmake --build --preset default
```

A single sample can also be built by target, for example:

```bash
cmake --build --preset default --target HelloTriangle
```

Prebuilt sample packages are also published with Elemental development releases.

## Elemental

These are the best place to start. Each sample introduces another part of the runtime API.

1. [`01-HelloTriangle`](Elemental/01-HelloTriangle) — create the basic graphics objects and draw a procedural triangle with a mesh shader.
2. [`02-HelloInputs`](Elemental/02-HelloInputs) — consume Elemental's input stream from keyboard, mouse, gamepad and touch devices.
3. [`03-HelloCompute`](Elemental/03-HelloCompute) — generate a Julia fractal with a compute shader, synchronize the texture and display it through the graphics pipeline.
4. [`04-HelloMesh`](Elemental/04-HelloMesh) — load a minimal compiled mesh buffer, use meshlets and render a real mesh with a depth buffer.
5. [`05-HelloRaytracing`](Elemental/05-HelloRaytracing) — build acceleration structures and use inline ray queries for interactive ray/path-tracing experiments.

## Demos

Demos are larger laboratories rather than minimal tutorials. They intentionally combine several systems and may contain active TODOs, experiments and temporary implementation choices.

- [`01-Renderer`](Demos/01-Renderer) — Elemental's main renderer stress test, combining scene loading, mesh shaders, bindless resources, ray tracing, compute path tracing, GPU timings and a debug UI.
- [`02-AITraining`](Demos/02-AITraining) — a small CPU-side neural-network/autodiff experiment used as a baseline for future GPU-compute exploration. It does **not** run neural-network training on the GPU yet.

## ElementalTools

These command-line samples demonstrate the offline side of the project.

- [`01-ShaderCompiler`](ElementalTools/01-ShaderCompiler) — compile HLSL into an Elemental shader library for a target backend/platform.
- [`02-SceneCompiler`](ElementalTools/02-SceneCompiler) — load a source scene, build meshlets and serialize the richer sample scene format.
- [`03-TextureCompiler`](ElementalTools/03-TextureCompiler) — load a texture, generate mip levels and write BC7-compressed sample texture data.
- [`04-MeshCompiler`](ElementalTools/04-MeshCompiler) — build the deliberately minimal meshlet data used by HelloMesh.

The `.scene`, `.mesh` and `.texture` files produced by these samples are formats used by the sample code. They should not be treated as stable public Elemental asset formats.
