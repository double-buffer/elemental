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

});

var currentRenderSize = applicationService.GetWindowRenderSize(window);

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

    graphicsService.WaitForSwapChainOnCpu(swapChain);

    var commandLists = new CommandList[5 * 5];
    
    var backbufferTexture = graphicsService.GetSwapChainBackBufferTexture(swapChain);

    Parallel.For (0, 5, (i) =>
    {
        for (var j = 0; j < 5; j++)
        {
            var commandList = graphicsService.CreateCommandList(commandQueue);
            graphicsService.SetCommandListLabel(commandList, $"Triangle CommandList {j} (Thread :{i})");

            graphicsService.BeginRenderPass(commandList, new RenderPassDescriptor
            {
                RenderTarget0 = new RenderPassRenderTarget
                {
                    Texture = backbufferTexture,
                    ClearColor = new Vector4(0.8f, 0.5f, 0.0f, 1.0f)
                }
            });

            // TODO: Add a counter in the mesh dispatch push constant to test the frame latency

            graphicsService.EndRenderPass(commandList);
           /* 
            graphicsService.BeginRenderPass(commandList, new RenderPassDescriptor
            {
                RenderTarget0 = new RenderPassRenderTarget
                {
                    Texture = graphicsService.GetSwapChainBackBufferTexture(swapChain),
                    ClearColor = new Vector4(0.0f, 1.0f, i == 4 ? 1.0f : 0.0f, 1.0f)
                }
            });

            graphicsService.EndRenderPass(commandList);*/

            graphicsService.CommitCommandList(commandList);

            commandLists[i * 5 + j] = commandList;
        }
    });

    var fence = graphicsService.ExecuteCommandLists(commandQueue, commandLists, Array.Empty<Fence>());

    graphicsService.WaitForFenceOnCpu(fence);

    graphicsService.PresentSwapChain(swapChain);

    return true;
});