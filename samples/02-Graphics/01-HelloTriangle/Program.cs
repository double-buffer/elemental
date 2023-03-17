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

Console.WriteLine($"Can Compile Shader HLSL: {shaderCompiler.CanCompileShader(ShaderLanguage.Hlsl, (ToolsGraphicsApi)selectedGraphicsDevice.GraphicsApi)}");
Console.WriteLine($"Can Compile Shader Metal: {shaderCompiler.CanCompileShader(ShaderLanguage.Msl, (ToolsGraphicsApi)selectedGraphicsDevice.GraphicsApi)}");

// TODO: Do a batch compile function?

var shaderCode = File.ReadAllText("Triangle.hlsl");
var shaderCodeMetal = File.ReadAllText("Triangle.metal");
var meshShaderSourceType = selectedGraphicsDevice.GraphicsApi == GraphicsApi.Metal ? ShaderLanguage.Msl : ShaderLanguage.Hlsl;

var compilationResult = shaderCompiler.CompileShader(selectedGraphicsDevice.GraphicsApi == GraphicsApi.Metal ? shaderCodeMetal : shaderCode, ToolsShaderStage.MeshShader, "MeshMain", meshShaderSourceType, (ToolsGraphicsApi)selectedGraphicsDevice.GraphicsApi);

foreach (var logEntry in compilationResult.LogEntries.Span)
{
    Console.WriteLine($"{logEntry.Type}: {logEntry.Message}");
}

if (!compilationResult.IsSuccess)
{
    return;
}

var meshShaderData = compilationResult.ShaderData;

compilationResult = shaderCompiler.CompileShader(shaderCode, ToolsShaderStage.PixelShader, "PixelMain", ShaderLanguage.Hlsl, (ToolsGraphicsApi)selectedGraphicsDevice.GraphicsApi);

foreach (var logEntry in compilationResult.LogEntries.Span)
{
    Console.WriteLine($"{logEntry.Type}: {logEntry.Message}");
}

if (!compilationResult.IsSuccess)
{
    return;
}

var pixelShaderData = compilationResult.ShaderData;

using var shader = graphicsService.CreateShader(graphicsDevice, new ShaderPart[]
{
    new ShaderPart { Stage = ShaderStage.MeshShader, EntryPoint = "MeshMain", Data = meshShaderData },
    new ShaderPart { Stage = ShaderStage.PixelShader, EntryPoint = "PixelMain", Data = pixelShaderData }
});

var stopWatch = new Stopwatch();
var shaderParameters = new ShaderParameters();

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
    shaderParameters = shaderParameters with { RotationY = shaderParameters.RotationY + 0.5f * deltaTime };

    var commandList = graphicsService.CreateCommandList(commandQueue);

    using var backbufferTexture = graphicsService.GetSwapChainBackBufferTexture(swapChain);
    graphicsService.BeginRenderPass(commandList, new() { RenderTarget0 = new() { Texture = backbufferTexture, ClearColor = new Vector4(0.0f, 1.0f, 1.0f, 1.0f) } });

    graphicsService.SetShader(commandList, shader);
    graphicsService.SetShaderConstants(commandList, 0, ref shaderParameters);
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

readonly record struct ShaderParameters(float RotationX, float RotationY);