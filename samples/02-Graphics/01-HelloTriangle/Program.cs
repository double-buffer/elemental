using System.Diagnostics;
using System.Numerics;
using Elemental;
using Elemental.Graphics;
using Elemental.Tools;

//TODO: HelloTriangle should be minimal, do another sample to demonstrate the debug functionality

var applicationService = new NativeApplicationService();
using var graphicsService = new GraphicsService(new() { GraphicsDiagnostics = GraphicsDiagnostics.Debug });

using var application = applicationService.CreateApplication("Hello Window");
using var window = applicationService.CreateWindow(application);

var availableGraphicsDevices = graphicsService.GetAvailableGraphicsDevices();
var selectedGraphicsDevice = availableGraphicsDevices[0];

// TODO: Do a separate sample to showcase the use of Native + Vulkan with 2 windows (if available)
foreach (var availableGraphicsDevice in availableGraphicsDevices)
{
    if (availableGraphicsDevice.GraphicsApi == GraphicsApi.Vulkan)
    {
        //selectedGraphicsDevice = availableGraphicsDevice;
    }

    Console.WriteLine($"{availableGraphicsDevice}");
}

using var graphicsDevice = graphicsService.CreateGraphicsDevice(new() { DeviceId = selectedGraphicsDevice.DeviceId });
using var commandQueue = graphicsService.CreateCommandQueue(graphicsDevice, CommandQueueType.Graphics);

applicationService.SetWindowTitle(window, $"Hello Triangle! (GraphicsDevice: {graphicsService.GetGraphicsDeviceInfo(graphicsDevice)})");

using var swapChain = graphicsService.CreateSwapChain(window, commandQueue);
var currentRenderSize = applicationService.GetWindowRenderSize(window);

// HACK: This is needed when we run the program from the root directory
Environment.CurrentDirectory = Path.GetDirectoryName(Environment.ProcessPath)!;

var shaderCompiler = new ShaderCompiler();
shaderCompiler.Hello();

Console.WriteLine($"Can Compile Shader HLSL: {shaderCompiler.CanCompileShader(ShaderLanguage.Hlsl, (ToolsGraphicsApi)selectedGraphicsDevice.GraphicsApi)}");
Console.WriteLine($"Can Compile Shader Metal: {shaderCompiler.CanCompileShader(ShaderLanguage.Msl, (ToolsGraphicsApi)selectedGraphicsDevice.GraphicsApi)}");

// TODO: Do a batch compile function?

var shaderCode = File.ReadAllText("Triangle.hlsl");

// TODO: This is needed for Apple Metal because SPIRV-Cross doesn't yet support metal mesh shaders
var shaderCodeMetal = File.ReadAllText("Triangle.metal");
var meshShaderSourceType = selectedGraphicsDevice.GraphicsApi == GraphicsApi.Metal ? ShaderLanguage.Msl : ShaderLanguage.Hlsl;
var meshShaderCompilationResult = shaderCompiler.CompileShader(selectedGraphicsDevice.GraphicsApi == GraphicsApi.Metal ? shaderCodeMetal : shaderCode, ToolsShaderStage.MeshShader, "MeshMain", meshShaderSourceType, (ToolsGraphicsApi)selectedGraphicsDevice.GraphicsApi);

foreach (var logEntry in meshShaderCompilationResult.LogEntries.Span)
{
    Console.WriteLine($"{logEntry.Type}: {logEntry.Message}");
}

if (!meshShaderCompilationResult.IsSuccess)
{
    return;
}

var pixelShaderCompilationResult = shaderCompiler.CompileShader(shaderCode, ToolsShaderStage.PixelShader, "PixelMain", ShaderLanguage.Hlsl, (ToolsGraphicsApi)selectedGraphicsDevice.GraphicsApi);

foreach (var logEntry in pixelShaderCompilationResult.LogEntries.Span)
{
    Console.WriteLine($"{logEntry.Type}: {logEntry.Message}");
}

if (!pixelShaderCompilationResult.IsSuccess)
{
    return;
}

using var shader = graphicsService.CreateShader(graphicsDevice, new ShaderPart[]
{
    new ShaderPart 
    { 
        Stage = (ShaderStage)meshShaderCompilationResult.Stage, 
        EntryPoint = meshShaderCompilationResult.EntryPoint, 
        Data = meshShaderCompilationResult.ShaderData, 
        MetaData = new ShaderPartMetaData[] 
        {
            new ShaderPartMetaData { Type = ShaderPartMetaDataType.PushConstantsCount, Value = 1 }, 
            new ShaderPartMetaData { Type = ShaderPartMetaDataType.ThreadCountX, Value = 32 },
            new ShaderPartMetaData { Type = ShaderPartMetaDataType.ThreadCountY, Value = 1 },
            new ShaderPartMetaData { Type = ShaderPartMetaDataType.ThreadCountZ, Value = 1 } 
        }
    },
    new ShaderPart 
    {
        Stage = (ShaderStage)pixelShaderCompilationResult.Stage, 
        EntryPoint = pixelShaderCompilationResult.EntryPoint, 
        Data = pixelShaderCompilationResult.ShaderData,
        MetaData = new ShaderPartMetaData[] 
        { 
            new ShaderPartMetaData { Type = ShaderPartMetaDataType.PushConstantsCount, Value = 1 } 
        }  
    }
});

var stopWatch = new Stopwatch();
var rotationY = 0.0f;

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing)
    {
        return false;
    }

    var renderSize = applicationService.GetWindowRenderSize(window);

    if (renderSize != currentRenderSize)
    {
        Console.WriteLine($"Resize SwapChain to: {renderSize}");
        graphicsService.ResizeSwapChain(swapChain, renderSize.Width, renderSize.Height);
        currentRenderSize = renderSize;
    }

    stopWatch.Restart();
    graphicsService.WaitForSwapChainOnCpu(swapChain);
    stopWatch.Stop();

    //Console.WriteLine($"Wait for swapchain {stopWatch.Elapsed.TotalMilliseconds}");

    // TODO: Compute real delta time
    var deltaTime = 1.0f / 60.0f;
    rotationY += 0.5f * deltaTime;

    var commandList = graphicsService.CreateCommandList(commandQueue);

    using var backbufferTexture = graphicsService.GetSwapChainBackBufferTexture(swapChain);
    graphicsService.BeginRenderPass(commandList, new() { RenderTarget0 = new() { Texture = backbufferTexture, ClearColor = new Vector4(0.0f, 1.0f, 1.0f, 1.0f) } });

    graphicsService.SetShader(commandList, shader);
    graphicsService.SetShaderConstants(commandList, 0, ref rotationY);
    graphicsService.DispatchMesh(commandList, 1, 1, 1);

    graphicsService.EndRenderPass(commandList);
    graphicsService.CommitCommandList(commandList);
    graphicsService.ExecuteCommandList(commandQueue, commandList);

    stopWatch.Restart();
    graphicsService.PresentSwapChain(swapChain);
    stopWatch.Stop();

    //Console.WriteLine($"Present swapchain {stopWatch.Elapsed.TotalMilliseconds}");
    return true;
});