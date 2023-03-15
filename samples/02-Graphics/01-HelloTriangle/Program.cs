using System.Diagnostics;
using System.Numerics;
using Elemental;
using Elemental.Graphics;

var applicationService = new NativeApplicationService();
using var application = applicationService.CreateApplication("Hello Window");

//TODO: HelloTriangle should be minimal, do another sample to demonstrate the debug functionality

using var window = applicationService.CreateWindow(application, new NativeWindowOptions
{
    Title = "Hello Window!",
    Width = 1280,
    Height = 720,
    WindowState = NativeWindowState.Normal
});

using var graphicsService = new GraphicsService(new GraphicsServiceOptions
{
    GraphicsDiagnostics = GraphicsDiagnostics.Debug
});

var availableGraphicsDevices = graphicsService.GetAvailableGraphicsDevices();
var selectedGraphicsDevice = availableGraphicsDevices[0];

foreach (var availableGraphicsDevice in availableGraphicsDevices)
{
    if (availableGraphicsDevice.GraphicsApi == GraphicsApi.Vulkan)
    {
        selectedGraphicsDevice = availableGraphicsDevice;
    }

    Console.WriteLine($"{availableGraphicsDevice}");
}

using var graphicsDevice = graphicsService.CreateGraphicsDevice(new GraphicsDeviceOptions
{
    DeviceId = selectedGraphicsDevice.DeviceId
});

using var commandQueue = graphicsService.CreateCommandQueue(graphicsDevice, CommandQueueType.Graphics);
graphicsService.SetCommandQueueLabel(commandQueue, "Test Render Queue");

var graphicsDeviceInfos = graphicsService.GetGraphicsDeviceInfo(graphicsDevice);
applicationService.SetWindowTitle(window, $"Hello Triangle! (GraphicsDevice: {graphicsDeviceInfos})");

using var swapChain = graphicsService.CreateSwapChain(window, commandQueue, new SwapChainOptions
{
    Format = SwapChainFormat.Default
});

var currentRenderSize = applicationService.GetWindowRenderSize(window);

Environment.CurrentDirectory = Path.GetDirectoryName(Environment.ProcessPath)!;

var shaderExtension = ".bin";

if (selectedGraphicsDevice.GraphicsApi == GraphicsApi.Vulkan)
{
    shaderExtension = ".vulkan.bin";
}

var meshShaderData = File.ReadAllBytes($"TriangleMS{shaderExtension}");
var pixelShaderData = File.ReadAllBytes($"TrianglePS{shaderExtension}");

using var shader = graphicsService.CreateShader(graphicsDevice, new ShaderPart[]
{
    new ShaderPart { Stage = ShaderStage.MeshShader, EntryPoint = "MeshMain", Data = meshShaderData },
    new ShaderPart { Stage = ShaderStage.PixelShader, EntryPoint = "PixelMain", Data = pixelShaderData }
});

var stopWatch = new Stopwatch();
var currentRotationX = 0.0f;
var currentRotationY = 0.0f;

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing)
    {
        Console.WriteLine("Closing Application...");
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

    using var backbufferTexture = graphicsService.GetSwapChainBackBufferTexture(swapChain);

    var commandList = graphicsService.CreateCommandList(commandQueue);
    graphicsService.SetCommandListLabel(commandList, $"Triangle CommandList)");

    graphicsService.BeginRenderPass(commandList, new RenderPassDescriptor
    {
        RenderTarget0 = new RenderPassRenderTarget
        {
            Texture = backbufferTexture,
            ClearColor = new Vector4(0.0f, 1.0f, 1.0f, 1.0f)
        }
    });

    graphicsService.SetShader(commandList, shader);

    // TODO: Compute real delta time
    var deltaTime = 1.0f / 60.0f;
    //currentRotationX += 0.5f * deltaTime;
    currentRotationY += 0.5f * deltaTime;

    var shaderParameters = new ShaderParameters() { RotationX = currentRotationX, RotationY = currentRotationY };
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

record struct ShaderParameters
{
    public float RotationX { get; set; }
    public float RotationY { get; set; }
}