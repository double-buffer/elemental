using System.Numerics;
using Elemental;
using Elemental.Graphics;

var applicationService = new NativeApplicationService();
using var application = applicationService.CreateApplication("Hello Window");

using var window = applicationService.CreateWindow(application, new NativeWindowOptions
{
    Title = "Hello Window!",
    Width = 1280,
    Height = 720
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

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing)
    {
        Console.WriteLine("Closing Application...");
        return false;
    }

    graphicsService.WaitForSwapChainOnCpu(swapChain);

    var renderSize = applicationService.GetWindowRenderSize(window);

    using var commandList = graphicsService.CreateCommandList(commandQueue);
    graphicsService.SetCommandListLabel(commandList, "Triangle CommandList");

    graphicsService.BeginRenderPass(commandList, new RenderPassDescriptor
    {
        RenderTarget0 = new RenderPassRenderTarget
        {
            Texture = graphicsService.GetSwapChainBackBufferTexture(swapChain),
            ClearColor = new Vector4(1.0f, 0.0f, 1.0f, 1.0f)
        }
    });

    graphicsService.EndRenderPass(commandList);

    graphicsService.CommitCommandList(commandList);

    var fence = graphicsService.ExecuteCommandList(commandQueue, commandList);
    Console.WriteLine($"Fence: {fence}");
    graphicsService.WaitForFenceOnCpu(fence);

    graphicsService.PresentSwapChain(swapChain);

    return true;
});