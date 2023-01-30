# Elemental ![License](https://img.shields.io/github/license/double-buffer/elemental.svg) [![NuGet](https://img.shields.io/nuget/v/Elemental.svg)](https://www.nuget.org/packages/Elemental/)

## 📖 Purpose

Elemental is a portable low-level game platform abstraction library for .NET 7+ that targets only next-gen features.

## 🚀 Getting Started

Open a command line, create a new console project and add the Elemental NuGet package.

```
dotnet new console
dotnet add package Elemental
```

Copy and paste this sample code to create an empty window and display its current render size in the title bar.

```
using Elemental;

var applicationService = new NativeApplicationService();
using var application = applicationService.CreateApplication("Hello Window");

using var window = applicationService.CreateWindow(application, new()
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

    Thread.Sleep(5);

    return true;
});
```

Run the app using on any supported platform. (Currently Windows and MacOS)

```
dotnet run
```

You will find more examples in the [samples folder](samples/readme.md).