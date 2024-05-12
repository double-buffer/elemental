# Elemental ![License](https://img.shields.io/github/license/double-buffer/elemental.svg) ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/double-buffer/elemental/build-ci.yml?branch=main)

## 📖 Purpose

Elemental is a portable low-level game platform abstraction library that targets only next-gen features. Back in the days of DirectX11 and OpenGL,
it was very easy to start working on its own custom program that took full advantage of the GPU. New apis give more control to the developper but 
they are hard to use. They also have many ways of doing things because they must maintain backward compatibility.

The goal of Elemental is to simplify and unify all graphics API on all platforms. It doesn't mean that it will simplify or abstract graphics development. 
It is still challenging but developers can now focus on learning graphics programming like in the old days. And build their graphics app or engine by focusing
on graphics algorithms.

The library is still in heavy development stage. The goal is not to be compatible with the majority of graphics cards now but to be ready for the future.

In order to simplify the API, the library makes the following assumptions:

- Use of Mesh shaders for the geometry pipeline: so no vertex layout setup, no geometry shaders, no vertex shaders.
- No copy queue exposed: you need to use IO queue to copy data from disk to GPU memory. (Internally the library try to use DirectStorage, MetalIO, etc.)
For dynamic gpu data that is short lived, we use GPU upload heap (rebar memory).
- Use of bindless resources
- No indirect commands: The future will be about work graphs. 

It currently runs on:

- Windows (Direct3D12, Vulkan)
- Linux (Vulkan)
- MacOS (Metal)
- iOS (Metal)

The library is exposed as a C library but other bindings will are available:

- dotnet: WIP
Elemental: [![NuGet](https://img.shields.io/nuget/v/Elemental.svg)](https://www.nuget.org/packages/Elemental/) 

Elemental Tools: [![NuGet](https://img.shields.io/nuget/v/Elemental.Tools.svg)](https://www.nuget.org/packages/Elemental.Tools/)

For shader development you can use the following languages:

HLSL | 

## 🚀 Getting Started

This getting started exemple, displays a colored triangle using a mesh shader.

![](/samples/screenshots/GettingStarted.png)

Open a command line, create a new console project and add the Elemental NuGet package.

```powershell
dotnet new console
dotnet add package Elemental --prerelease
dotnet add package Elemental.Tools --prerelease
```

Copy and paste this sample code to create an empty window and display its current render size in the title bar.

```csharp
using System.Numerics;
using Elemental;
using Elemental.Graphics;
using Elemental.Tools;

var applicationService = new NativeApplicationService();
using var graphicsService = new GraphicsService();

using var application = applicationService.CreateApplication("Hello Window");
using var window = applicationService.CreateWindow(application);

using var graphicsDevice = graphicsService.CreateGraphicsDevice();
using var commandQueue = graphicsService.CreateCommandQueue(graphicsDevice, CommandQueueType.Graphics);
using var swapChain = graphicsService.CreateSwapChain(window, commandQueue);
var currentRenderSize = applicationService.GetWindowRenderSize(window);

var shaderCode = 
    """
    #define RootSignatureDef "RootFlags(0)"

    struct Vertex { float3 Position; float4 Color; };
    struct VertexOutput { float4 Position: SV_Position; float4 Color: TEXCOORD0; };

    static Vertex triangleVertices[] =
    {
        { float3(-0.5, 0.5, 0.0), float4(1.0, 0.0, 0.0, 1.0) },
        { float3(0.5, 0.5, 0.0), float4(0.0, 1.0, 0.0, 1.0) },
        { float3(-0.5, -0.5, 0.0), float4(0.0, 0.0, 1.0, 1.0) }
    };

    [OutputTopology("triangle")]
    [NumThreads(32, 1, 1)]
    void MeshMain(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[3], out indices uint3 indices[1])
    {
        const uint meshVertexCount = 3;

        SetMeshOutputCounts(meshVertexCount, 1);

        if (groupThreadId < meshVertexCount)
        {
            vertices[groupThreadId].Position = float4(triangleVertices[groupThreadId].Position, 1.0);
            vertices[groupThreadId].Color = triangleVertices[groupThreadId].Color;
        }

        if (groupThreadId == 0)
        {
            indices[groupThreadId] = uint3(0, 1, 2);
        }
    }

    float4 PixelMain(const VertexOutput input) : SV_Target0
    {
        return input.Color; 
    }
    """;

using var shaderCompiler = new ShaderCompiler();

var shaderInputs = new ShaderCompilerInput[]
{
    new() { ShaderCode = shaderCode, Stage = ShaderStage.MeshShader, EntryPoint = "MeshMain", ShaderLanguage = ShaderLanguage.Hlsl },
    new() { ShaderCode = shaderCode, Stage = ShaderStage.PixelShader, EntryPoint = "PixelMain", ShaderLanguage = ShaderLanguage.Hlsl }
};

var shaderCompilationResults = shaderCompiler.CompileShaders(shaderInputs, graphicsService.GetGraphicsDeviceInfo(graphicsDevice).GraphicsApi);
var shaderParts = new ShaderPart[shaderCompilationResults.Length];

for (var i = 0; i < shaderCompilationResults.Length; i++)
{
    shaderParts[i] = new() 
    { 
        Stage = shaderCompilationResults[i].Stage, 
        EntryPoint = shaderCompilationResults[i].EntryPoint, 
        Data = shaderCompilationResults[i].ShaderData, 
        MetaData = shaderCompilationResults[i].MetaData
    };
}

using var shader = graphicsService.CreateShader(graphicsDevice, shaderParts);

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing) return false;

    var renderSize = applicationService.GetWindowRenderSize(window);

    if (renderSize != currentRenderSize)
    {
        graphicsService.ResizeSwapChain(swapChain, renderSize.Width, renderSize.Height);
        currentRenderSize = renderSize;
    }

    graphicsService.WaitForSwapChainOnCpu(swapChain);

    var commandList = graphicsService.CreateCommandList(commandQueue);
    using var backbufferTexture = graphicsService.GetSwapChainBackBufferTexture(swapChain);
    graphicsService.BeginRenderPass(commandList, new() { RenderTarget0 = new() { Texture = backbufferTexture, ClearColor = new Vector4(0.0f, 1.0f, 1.0f, 1.0f) } });
    graphicsService.SetShader(commandList, shader);
    graphicsService.DispatchMesh(commandList, 1, 1, 1);
    graphicsService.EndRenderPass(commandList);
    graphicsService.CommitCommandList(commandList);
    graphicsService.ExecuteCommandList(commandQueue, commandList);

    graphicsService.PresentSwapChain(swapChain);
    return true;
});
```

For MacOS, put those lines into the csproj:

```xml
  <PropertyGroup Condition="$([MSBuild]::IsOSPlatform('OSX'))">
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
    <OutputPath>./bin/$(Configuration)/$(AssemblyName).app/Contents/MacOS</OutputPath>
  </PropertyGroup>
```

Run the app using on any supported platform. (Currently Windows and MacOS)

```
dotnet run
```

Warning: Please note that that for the moment this getting started code will not run on MacOS. It is because SPIRV-Cross doesn't support yet Metal Mesh Shader. 

In the near future, it will work on MacOS too. For a work around see the sample HelloTriangle that use a metal shader for the mesh shader.

You will find more examples in the [samples folder](samples).
