# Shader Compiler

A minimal command-line example of offline shader compilation with ElementalTools.

The tool reads an HLSL source file, compiles it for an Elemental graphics backend/platform and writes the resulting shader-library binary to disk.

## Usage

```text
ShaderCompiler [options] inputfile outputfile
```

Example:

```bash
ShaderCompiler MyShader.hlsl MyShader.shader
```

Host defaults are:

| Host | Default target |
|:--|:--|
| Windows | Direct3D 12 / Windows |
| Linux | Vulkan / Linux |
| macOS | Metal / macOS |

The current sample parser also handles:

- `--debug` — include shader debug information;
- `--target-api vulkan` — select Vulkan;
- `--target-platform iOS` — target iOS.

The parser is intentionally basic and does not yet expose every target combination supported by ElementalTools.

## Flow

```text
HLSL source
    -> ElemCompileShaderLibrary
    -> compiler messages / errors
    -> compiled shader-library data
    -> output file
```

The generated binary can be loaded at runtime with `ElemCreateShaderLibrary`.

## Key file

- [`main.c`](main.c) — argument parsing, shader compilation and output writing.

## Build

```bash
cmake --build --preset default --target ShaderCompiler
```

This command-line sample is not built for iOS.

[Back to samples](../../README.md)
