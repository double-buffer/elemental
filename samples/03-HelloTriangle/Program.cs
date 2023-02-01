using Elemental;
using Elemental.Graphics;

var applicationService = new NativeApplicationService();
using var graphicsService = new GraphicsService();

using var application = applicationService.CreateApplication("Hello Window");

using var window = applicationService.CreateWindow(application, new NativeWindowOptions
{
    Title = "Hello Window!",
    Width = 1280,
    Height = 720
});

using var graphicsDevice = graphicsService.CreateGraphicsDevice(new GraphicsDeviceOptions
{
    GraphicsDiagnostics = GraphicsDiagnostics.Debug
});

var graphicsDeviceInfos = graphicsService.GetGraphicsDeviceInfo(graphicsDevice);

applicationService.SetWindowTitle(window, $"Hello Triangle! (GraphicsDevice: {graphicsDeviceInfos})");

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing)
    {
        Console.WriteLine("Closing Application...");
    }

    var renderSize = applicationService.GetWindowRenderSize(window);

    Thread.Sleep(5);

    return true;
});