﻿using Elemental;

var applicationService = new NativeApplicationService();
var application = applicationService.CreateApplication("Hello Window");

var window = applicationService.CreateWindow(application, new NativeWindowDescription
{
    Title = "Hello Window!",
    Width = 1280,
    Height = 720
});

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing)
    {
        Console.WriteLine("Closing Application...");
    }

    var renderSize = applicationService.GetWindowRenderSize(window);
    applicationService.SetWindowTitle(window, $"Hello window! (Current RenderSize: {renderSize})");

    return true;
});