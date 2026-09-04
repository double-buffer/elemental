# Hello Mesh

Hello Mesh moves from procedural geometry to a real mesh while keeping the data flow deliberately small and explicit.

A dedicated [`MeshCompiler`](../../ElementalTools/04-MeshCompiler) converts the first mesh primitive from an OBJ or glTF source into a minimal `.mesh` file. At runtime, the sample reads that file, uploads its contiguous payload into one GPU buffer and passes the buffer descriptor plus four offsets to the mesh shader.

## What it demonstrates

- compiling a source mesh into meshlets with ElementalTools;
- loading a deliberately minimal sample `.mesh` format;
- creating one GPU-upload heap and one bindless mesh buffer;
- uploading vertex, meshlet and meshlet-index data as one contiguous payload;
- passing explicit buffer offsets to a mesh shader;
- creating and resizing a depth buffer;
- configuring depth testing;
- dispatching one mesh-shader workgroup per meshlet;
- interactive model-viewer controls.

The runtime sample deliberately has no scene graph, materials or texture model. Those concepts belong to larger samples such as the Renderer; they are not required to explain a mesh shader.

The current sample expects `kitten.mesh`. With a local `kitten.obj` or `kitten.gltf` in `Data`, the resource build invokes `MeshCompiler` and produces that file automatically.

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

- [`main.c`](main.c) — reads the mesh header, creates the GPU buffer and dispatches the mesh shader.
- [`Data/RenderMesh.hlsl`](Data/RenderMesh.hlsl) — reads the explicit mesh-buffer regions from the bindless buffer.
- [`../../ElementalTools/04-MeshCompiler`](../../ElementalTools/04-MeshCompiler) — creates the minimal mesh data consumed here.

## Build

```bash
cmake --build --preset default --target HelloMesh
```

Supported sample flags include `--vulkan`, `--fullscreen` and `--gpu-debug`.

> [!NOTE]
> The `.mesh` layout exists to keep this sample easy to understand. It is not a stable public Elemental asset format or a replacement for the richer sample scene pipeline.

[Back to samples](../../README.md)
