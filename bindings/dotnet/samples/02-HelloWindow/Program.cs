using Elemental;

var applicationService = new ApplicationService();
applicationService.ConfigureLogHandler(DefaultLogHandlers.ConsoleLogHandler);

using var application = applicationService.CreateApplication("Hello Window"u8);

using var window = applicationService.CreateWindow(application, new WindowOptions
{
    Title = "Hello Window!"u8
});

applicationService.RunApplication(application, (status) =>
{
    if (status == ApplicationStatus.Closing)
    {
        Console.WriteLine("Closing Application...");
        return false;
    }

    var renderSize = applicationService.GetWindowRenderSize(window);
    applicationService.SetWindowTitle(window, $"Hello window! (Current RenderSize: Width={renderSize.Width}, Height={renderSize.Height}, UIScale={renderSize.UIScale}, State={renderSize.WindowState})");

    Thread.Sleep(5);

    return true;
});
