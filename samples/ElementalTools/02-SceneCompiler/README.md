# Scene Compiler

A command-line example that turns a source scene into the compact binary scene format used by Elemental's samples.

It uses ElementalTools to load the source scene, builds meshlets for every mesh primitive and serializes the data needed by the mesh and ray-tracing samples.

## Usage

```text
SceneCompiler [options] inputfile outputfile
```

Example:

```bash
SceneCompiler model.gltf model.scene
```

The current option parser is still experimental; the `--meshlet-triangle-count` entry printed by the tool is a placeholder and is not implemented yet.

## What it does

For each scene, the sample:

- loads meshes, materials and nodes with `ElemLoadScene`;
- builds meshlets with `ElemBuildMeshlets`;
- packs vertex and index data;
- packs meshlet metadata, meshlet vertex indices and meshlet triangle indices;
- records material references and a small texture table;
- writes offsets so the runtime sample loader can access each data block directly.

Conceptually:

```text
source scene
    -> ElementalTools scene loader
    -> meshlet generation
    -> sample-specific packing
    -> .scene file
```

The generated files are consumed by samples such as Hello Mesh, Hello Raytracing and Renderer.

> [!NOTE]
> `.scene` is a format owned by the sample code. It is not a stable public Elemental asset format or a general-purpose scene standard.

The sample also contains a few hard-coded import adjustments for known test assets while the asset pipeline is still evolving.

## Key file

- [`main.c`](main.c) — scene import, meshlet generation and binary serialization.

## Build

```bash
cmake --build --preset default --target SceneCompiler
```

This command-line sample is not built for iOS.

[Back to samples](../../README.md)
