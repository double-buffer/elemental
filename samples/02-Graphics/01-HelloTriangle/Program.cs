using System.Diagnostics;
using System.Numerics;
using Elemental;
using Elemental.Graphics;

var applicationService = new NativeApplicationService();
using var application = applicationService.CreateApplication("Hello Window");

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
    //DeviceId = selectedGraphicsDevice.DeviceId
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

var meshShaderData = File.ReadAllBytes("TriangleMS.bin");
var pixelShaderData = File.ReadAllBytes("TrianglePS.bin");

using var shader = graphicsService.CreateShader(graphicsDevice, new ShaderPart[]
{
    new ShaderPart { Stage = ShaderStage.MeshShader, Data = meshShaderData },
    new ShaderPart { Stage = ShaderStage.PixelShader, Data = pixelShaderData }
});

var stopWatch = new Stopwatch();

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