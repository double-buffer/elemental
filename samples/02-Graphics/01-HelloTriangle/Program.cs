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

using var swapChain = graphicsService.CreateSwapChain(window, commandQueue);
var currentRenderSize = applicationService.GetWindowRenderSize(window);

var stopwatch = new System.Diagnostics.Stopwatch();
stopwatch.Start();

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing)
    {
        Console.WriteLine("Closing Application...");
        return false;
    }

    if (stopwatch.ElapsedMilliseconds > 5000)
    {
        applicationService.SetWindowState(window, NativeWindowState.FullScreen);
        stopwatch.Reset();
        stopwatch.Stop();
    }
    
    var renderSize = applicationService.GetWindowRenderSize(window);

    if (renderSize != currentRenderSize)
    {
        Console.WriteLine($"Resize SwapChain to: {renderSize}");
        graphicsService.ResizeSwapChain(swapChain, renderSize.Width, renderSize.Height);
        currentRenderSize = renderSize;
    }

    graphicsService.WaitForSwapChainOnCpu(swapChain);
    
    using var commandList = graphicsService.CreateCommandList(commandQueue);
    graphicsService.SetCommandListLabel(commandList, "Triangle CommandList");

    graphicsService.BeginRenderPass(commandList, new RenderPassDescriptor
    {
        RenderTarget0 = new RenderPassRenderTarget
        {
            Texture = graphicsService.GetSwapChainBackBufferTexture(swapChain),
            ClearColor = new Vector4(0.8f, 0.5f, 0.0f, 1.0f)
        }
    });

    // TODO: Add a counter in the mesh dispatch push constant to test the frame latency

    graphicsService.EndRenderPass(commandList);

    graphicsService.CommitCommandList(commandList);

    var fence = graphicsService.ExecuteCommandList(commandQueue, commandList);
    graphicsService.WaitForFenceOnCpu(fence);

    graphicsService.PresentSwapChain(swapChain);

    return true;
});