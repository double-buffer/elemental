# Texture Compiler

A command-line example that converts image data into the binary texture format used by Elemental's samples.

The tool loads the source texture through ElementalTools, generates a mip chain when necessary and stores BC7-compressed mip data for runtime use.

## Usage

```text
TextureCompiler [options] inputfile outputfile
```

Example:

```bash
TextureCompiler albedo.png albedo.texture
```

There are no meaningful command-line options implemented yet; the parser currently only reserves space for future options.

## What it does

```text
source image
    -> ElemLoadTexture
    -> generate mip levels when needed
    -> BC7 compression
    -> write mip offsets + data
    -> .texture file
```

If the loaded texture contains only one mip level, the sample calls `ElemGenerateTextureMipData` to build the rest of the chain. Each mip is then compressed with `ElemCompressTextureMipData` unless the input data is already BC7.

The output format is currently hard-coded to BC7.

> [!NOTE]
> `.texture` is a format owned by the sample code. It is not a stable public Elemental asset format or a general-purpose texture container.

## Key file

- [`main.c`](main.c) — texture loading, mip generation, compression and binary serialization.

## Build

```bash
cmake --build --preset default --target TextureCompiler
```

This command-line sample is not built for iOS.

[Back to samples](../../README.md)
