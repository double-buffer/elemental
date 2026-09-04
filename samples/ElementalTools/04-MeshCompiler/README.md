# Mesh Compiler

Mesh Compiler is the small offline companion to [`HelloMesh`](../../Elemental/04-HelloMesh).

It loads an OBJ or glTF source through ElementalTools, takes the first mesh primitive, builds meshlets and writes the minimal `.mesh` data consumed by the sample.

The output deliberately contains only what `HelloMesh` needs:

1. the remapped vertex buffer;
2. the `ElemMeshlet` array;
3. meshlet vertex indices;
4. packed meshlet triangle indices;
5. a small header containing the offsets and meshlet count.

This keeps the mesh-shader sample independent from the richer sample `.scene` model and makes the CPU-to-GPU data flow explicit.

## Usage

```bash
MeshCompiler model.obj model.mesh
```

The current compiler intentionally uses only the first mesh primitive from the source file. It also expects the simple position/normal/tangent/UV vertex layout used by `HelloMesh`.

> [!NOTE]
> `.mesh` is a sample format, not a stable public Elemental asset format.

[Back to samples](../../README.md)
