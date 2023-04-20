using System.Diagnostics;
using System.Numerics;
using Elemental;
using Elemental.Graphics;
using Elemental.Inputs;
using Elemental.Tools;

using var applicationService = new NativeApplicationService();
using var graphicsService = new GraphicsService(new() { GraphicsDiagnostics = GraphicsDiagnostics.Debug });
using var inputsService = new InputsService();

using var application = applicationService.CreateApplication("Hello Window");
using var window = applicationService.CreateWindow(application);

var availableGraphicsDevices = graphicsService.GetAvailableGraphicsDevices();
var selectedGraphicsDevice = availableGraphicsDevices[0];

// TODO: Do a separate sample to showcase the use of Native + Vulkan with 2 windows (if available)
foreach (var availableGraphicsDevice in availableGraphicsDevices)
{
    if (availableGraphicsDevice.GraphicsApi == GraphicsApi.Vulkan)
    {
        selectedGraphicsDevice = availableGraphicsDevice;
    }

    Console.WriteLine($"{availableGraphicsDevice}");
}

using var graphicsDevice = graphicsService.CreateGraphicsDevice(new() { DeviceId = selectedGraphicsDevice.DeviceId });
using var commandQueue = graphicsService.CreateCommandQueue(graphicsDevice, CommandQueueType.Graphics);

applicationService.SetWindowTitle(window, $"Hello Inputs! (GraphicsDevice: {graphicsService.GetGraphicsDeviceInfo(graphicsDevice)})");

using var swapChain = graphicsService.CreateSwapChain(window, commandQueue, new() { MaximumFrameLatency = 1 });
var currentRenderSize = applicationService.GetWindowRenderSize(window);

// HACK: This is needed when we run the program from the root directory
Environment.CurrentDirectory = Path.GetDirectoryName(Environment.ProcessPath)!;

using var shaderCompiler = new ShaderCompiler();

var shaderCode = File.ReadAllText("Triangle.hlsl");

// TODO: This is needed for Apple Metal because SPIRV-Cross doesn't yet support metal mesh shaders
var shaderCodeMetal = File.ReadAllText("Triangle.metal");

var meshShaderSourceType = selectedGraphicsDevice.GraphicsApi == GraphicsApi.Metal ? ShaderLanguage.Msl : ShaderLanguage.Hlsl;

var shaderInputs = new ShaderCompilerInput[]
{
    new() { ShaderCode = selectedGraphicsDevice.GraphicsApi == GraphicsApi.Metal ? shaderCodeMetal : shaderCode, Stage = ShaderStage.MeshShader, EntryPoint = "MeshMain", ShaderLanguage = meshShaderSourceType },
    new() { ShaderCode = shaderCode, Stage = ShaderStage.PixelShader, EntryPoint = "PixelMain", ShaderLanguage = ShaderLanguage.Hlsl }
};

var shaderCompilationResults = shaderCompiler.CompileShaders(shaderInputs, selectedGraphicsDevice.GraphicsApi);

// TODO: Avoid the array declaration
var shaderParts = new ShaderPart[shaderCompilationResults.Length];

for (var i = 0; i < shaderCompilationResults.Length; i++)
{
    var shaderCompilationResult = shaderCompilationResults[i];

    foreach (var logEntry in shaderCompilationResult.LogEntries.Span)
    {
        Console.WriteLine($"{logEntry.Type}: {logEntry.Message}");
    }

    if (!shaderCompilationResult.IsSuccess)
    {
        return;
    }

    shaderParts[i] = new()
    {
        Stage = shaderCompilationResult.Stage, 
        EntryPoint = shaderCompilationResult.EntryPoint, 
        Data = shaderCompilationResult.ShaderData, 
        MetaData = !(selectedGraphicsDevice.GraphicsApi == GraphicsApi.Metal && shaderCompilationResult.Stage == ShaderStage.MeshShader) ? shaderCompilationResult.MetaData : new ShaderMetaData[] 
        {
            new ShaderMetaData { Type = ShaderMetaDataType.PushConstantsCount, Value = 1 }, 
            new ShaderMetaData { Type = ShaderMetaDataType.ThreadCountX, Value = 32 },
            new ShaderMetaData { Type = ShaderMetaDataType.ThreadCountY, Value = 1 },
            new ShaderMetaData { Type = ShaderMetaDataType.ThreadCountZ, Value = 1 } 
        }
    };
}
    
using var shader = graphicsService.CreateShader(graphicsDevice, shaderParts);

var frameTimer = new Stopwatch();
var shaderParameters = new ShaderParameters()
{
    AspectRatio = (float)currentRenderSize.Width / currentRenderSize.Height
};

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

        shaderParameters = shaderParameters with
        {
            AspectRatio = (float)currentRenderSize.Width / currentRenderSize.Height
        };
    }

    graphicsService.WaitForSwapChainOnCpu(swapChain);
    var inputState = inputsService.GetInputState(application);

    //Console.WriteLine($"GamePad LeftX: {inputState.Gamepad.LeftStickX.Value}");

    foreach (var state in inputState.InputStateData)
    {
        //Console.Write($"{state} ");
    }

    //Console.WriteLine();

    foreach (var inputObject in inputState.InputObjects)
    {
        //Console.WriteLine($"{inputObject} ");
    }

    shaderParameters = shaderParameters with
    {
        RotationX = shaderParameters.RotationX + -inputState.Gamepad.LeftStickY.Value * 5.0f * (float)frameTimer.Elapsed.TotalSeconds,
        RotationY = shaderParameters.RotationY + inputState.Gamepad.LeftStickX.Value * 5.0f * (float)frameTimer.Elapsed.TotalSeconds,
        CurrentColorIndex = inputState.Gamepad.Button1.Value == 1 ? shaderParameters.CurrentColorIndex + 1 : shaderParameters.CurrentColorIndex
    };

    //Console.WriteLine($"Left: {inputState.Gamepad.LeftStickX.Value}");

    frameTimer.Restart();

    var commandList = graphicsService.CreateCommandList(commandQueue);

    using var backbufferTexture = graphicsService.GetSwapChainBackBufferTexture(swapChain);
    graphicsService.BeginRenderPass(commandList, new() { RenderTarget0 = new() { Texture = backbufferTexture, ClearColor = new Vector4(0.0f, 0.0f, 0.0f, 1.0f) } });

    graphicsService.SetShader(commandList, shader);
    graphicsService.SetShaderConstants(commandList, 0, ref shaderParameters);
    graphicsService.DispatchMesh(commandList, 1, 1, 1);

    graphicsService.EndRenderPass(commandList);
    graphicsService.CommitCommandList(commandList);
    graphicsService.ExecuteCommandList(commandQueue, commandList);

    graphicsService.PresentSwapChain(swapChain);
    return true;
});

readonly record struct ShaderParameters
{
    public float RotationX { get; init; }
    public float RotationY { get; init; }
    public float RotationZ { get; init; }
    public float AspectRatio { get; init; }
    public uint CurrentColorIndex { get; init; }
}