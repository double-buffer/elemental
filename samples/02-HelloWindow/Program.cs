using Elemental;

var applicationService = new NativeApplicationService();
var application = applicationService.CreateApplication("Hello Window");

// TODO: DPI on MacOS
var window = applicationService.CreateWindow(application, new NativeWindowDescription
{
    Title = "Hello Window!",
    Width = 1280,
    Height = 720,
    IsDpiAware = true
});

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing)
    {
        Console.WriteLine("Closing Application...");
    }

    return true;
});