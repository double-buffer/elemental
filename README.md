# Elemental ![License](https://img.shields.io/github/license/double-buffer/elemental.svg) ![GitHub Repo stars](https://img.shields.io/github/stars/double-buffer/elemental?style=flat) [![GitHub Release Downloads](https://img.shields.io/github/downloads/double-buffer/elemental/total)](https://github.com/double-buffer/elemental/releases) ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/double-buffer/elemental/build-ci.yml?branch=main)

Elemental is a portable low-level game platform library that targets only next-gen features. 
It is exposed as a simple C header and other language bindings also exist.

It is a great choice if you want to:
- Create your own game engine.
- Learn game programming.
- Do graphics research projects with the latest available tech.

> [!WARNING]
> The project is currently in heavy development. API may have a lot of breaking changes until the first release.

## 📖 Purpose

Elemental aims to simplify and standardize game development across different platforms by offering a unified API.

Back in the days of DirectX11 and OpenGL, it was very easy to start working on its own game or custom engine that took full advantage of the GPU. 
Newer apis give more control to the developpers but they are hard to use. They also provide many ways of doing things because they must maintain backward compatibility.

Elemental bridges this gap by focusing on powerful, next-gen features without the overhead of backward compatibility. 
This approach not only facilitates learning and using graphics programming as in earlier times but also gears developers up for future advancements without compromising on performance.

It doesn't mean that it will simplify or abstract graphics development. 
It is a challenging topic but developers can now focus on learning graphics programming like in the old days by focusing on graphics algorithms and not on the complicated platform specific implementation details.

The library is still in heavy development stage. The goal is not to be compatible with the majority of graphics cards now but to be ready for the future.

In order to simplify the API, the library makes the following assumptions:

- Use of Mesh shaders for the geometry pipeline: so no vertex layout setup, no geometry shaders, no vertex shaders.
- No copy queue exposed: you need to use IO queue to copy data from disk to GPU memory. (Internally the library try to use DirectStorage, MetalIO, etc.)
For dynamic gpu data that is short lived, we use GPU upload heap (rebar memory).
- Use of bindless resources
- No indirect commands: The future will be about work graphs. 

## 📋 Features

Implemented features for current version **1.0.0-DEV4**:

- Application: 
    - Application Lifetime with the same code for all platforms.
    - Windows.
- Graphics:
    - Graphics Device.
    - CommandQueues and CommandLists.
    - SwapChain.
    - Mesh shaders.
- Inputs:
    - Keyboard.
    - Mouse.
    - Gamepad. (basic for now only work with Xbox one S wireless controller)
    - Touch. (iOS and MacOS)
- Tools:
    - Shader Compiler.

It currently runs on:

| Platform | Graphics API      | Tools Supported |
|:--------:|:-----------------:|:---------------:|
| <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/windows-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/windows-light.svg"><img alt="Linux" width="20pt"></picture> | DirectX12, Vulkan | ✅              |
| <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/linux-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/linux-light.svg"><img alt="Linux" width="20pt"></picture> | Vulkan/Wayland    | ✅              |
| <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/apple-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/apple-light.svg"><img alt="MacOS" width="20pt"></picture> | Metal3            | ✅              |
| <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/ios-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/ios-light.svg"><img alt="iOS" width="20pt"></picture> | Metal3            |                 |

The library is exposed as a C library but other bindings will be available:

| Binding | Status | Download      |
|:-------:|:------:|:-------------:|
| <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/dotnet-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/dotnet-light.svg"><img alt=".NET" width="20pt"></picture>    | WIP    | Not available |

For shader development, the shader compiler supports the following languages and platforms:

| Shader Language | Target API | Compilation Platforms |
|:---------------:|:----------:|:---------------------:|
| HLSL            | DirectX12  | <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/windows-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/windows-light.svg"><img alt="Linux" width="20pt"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/linux-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/linux-light.svg"><img alt="Linux" width="20pt"></picture> |
| HLSL            | Vulkan     | <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/windows-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/windows-light.svg"><img alt="Linux" width="20pt"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/linux-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/linux-light.svg"><img alt="Linux" width="20pt"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/apple-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/apple-light.svg"><img alt="MacOS" width="20pt"></picture> |
| HLSL            | Metal3     | <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/windows-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/windows-light.svg"><img alt="Linux" width="20pt"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="/doc/icons/apple-dark.svg"><source media="(prefers-color-scheme: light)" srcset="/doc/icons/apple-light.svg"><img alt="MacOS" width="20pt"></picture> |

## 🚀 Getting Started

Here is a minimal application that displays a colored triangle using a mesh shader.

![Minimal Triangle Application](/doc/GettingStarted.png)

```c
#include "Elemental.h"
#include "ElementalTools.h"

const char* shaderSource = 
    "struct Vertex { float3 Position; float4 Color; };"
    "struct VertexOutput { float4 Position: SV_Position; float4 Color: TEXCOORD0; };"

    "static Vertex triangleVertices[] ="
    "{"
    "    { float3(-0.5, 0.5, 0.0), float4(1.0, 0.0, 0.0, 1.0) },"
    "    { float3(0.5, 0.5, 0.0), float4(0.0, 1.0, 0.0, 1.0) },"
    "    { float3(-0.5, -0.5, 0.0), float4(0.0, 0.0, 1.0, 1.0) }"
    "};"

    "[shader(\"mesh\")]"
    "[OutputTopology(\"triangle\")]"
    "[NumThreads(32, 1, 1)]"
    "void MeshMain(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[3], out indices uint3 indices[1])"
    "{"
    "    const uint meshVertexCount = 3;"
    "    SetMeshOutputCounts(meshVertexCount, 1);"

    "    if (groupThreadId < meshVertexCount)"
    "    {"
    "        vertices[groupThreadId].Position = float4(triangleVertices[groupThreadId].Position, 1);"
    "        vertices[groupThreadId].Color = triangleVertices[groupThreadId].Color;"
    "    }"

    "    if (groupThreadId == 0)"
    "    {"
    "        indices[groupThreadId] = uint3(0, 1, 2);"
    "    }"
    "}"

    "[shader(\"pixel\")]"
    "float4 PixelMain(const VertexOutput input) : SV_Target0"
    "{"
    "    return input.Color;"
    "}";

typedef struct
{
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
} ApplicationPayload;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    applicationPayload->Window = ElemCreateWindow(NULL);

    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);
    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .UpdatePayload = payload });

    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);
    ElemSystemInfo systemInfo = ElemGetSystemInfo();
    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(applicationPayload->GraphicsDevice);

    ElemShaderSourceData shaderSourceData = { .ShaderLanguage = ElemShaderLanguage_Hlsl, .Data = { .Items = (uint8_t*)shaderSource, .Length = strlen(shaderSource) } };
    ElemShaderCompilationResult compilationResult = ElemCompileShaderLibrary((ElemToolsGraphicsApi)graphicsDeviceInfo.GraphicsApi, (ElemToolsPlatform)systemInfo.Platform, &shaderSourceData, NULL);

    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, (ElemDataSpan) { .Items = compilationResult.Data.Items, .Length = compilationResult.Data.Length });

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .TextureFormats = { .Items = (ElemTextureFormat[]) { swapChainInfo.Format }, .Length = 1 }
    });
    
    ElemFreeShaderLibrary(shaderLibrary);
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) {{ 
                .RenderTarget = updateParameters->BackBufferTexture, 
                .LoadAction = ElemRenderPassLoadAction_Clear 
            }},
            .Length = 1
        }
    });

    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline);
    ElemDispatchMesh(commandList, 1, 1, 1);
    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);
}

int main(int argc, const char* argv[]) 
{
    ElemRunApplication(&(ElemRunApplicationParameters) {
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &(ApplicationPayload){}
    });
}
```

> [!TIP]
> Please note that this code uses both Elemental and ElementalTools. In a real world app, you should compile your shaders offline.

> [!WARNING]
> This code will only runs on platforms that support shader compilation. (Windows, MacOS, Linux) 
> If you want to run code on other platforms you should compile your shaders offline.

You will find detailled examples in the [samples folder](samples).
