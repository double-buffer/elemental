namespace Elemental;

/// <inheritdoc />
public class ApplicationService : IApplicationService
{
    /// <summary>
    /// Configures a custom log handler for the application.
    /// </summary>
    /// <param name="logHandler">The log handler function to be used.</param>
    public void ConfigureLogHandler(LogHandler logHandler)
    {
        ApplicationServiceInterop.ConfigureLogHandler(logHandler);
    }

    /// <summary>
    /// Creates a new application instance.
    /// </summary>
    /// <param name="applicationName">The name of the application.</param>
    /// <returns>Returns an application handle.</returns>
    public ElementalApplication CreateApplication(ReadOnlySpan<byte> applicationName)
    {
        return ApplicationServiceInterop.CreateApplication(applicationName);
    }

    /// <summary>
    /// Creates a new application instance.
    /// </summary>
    /// <param name="applicationName">The name of the application.</param>
    /// <returns>Returns an application handle.</returns>
    public ElementalApplication CreateApplication(string applicationName)
    {
        return ApplicationServiceInterop.CreateApplication(Encoding.UTF8.GetBytes(applicationName));
    }

    /// <summary>
    /// Frees resources associated with the given application.
    /// </summary>
    /// <param name="application">The application to free.</param>
    public void FreeApplication(ElementalApplication application)
    {
        ApplicationServiceInterop.FreeApplication(application);
    }

    /// <summary>
    /// Runs the specified application with the provided run handler.
    /// </summary>
    /// <param name="application">The application to run.</param>
    /// <param name="runHandler">The function to call on each run iteration.</param>
    public void RunApplication(ElementalApplication application, RunHandler runHandler)
    {
        ApplicationServiceInterop.RunApplication(application, runHandler);
    }

    public unsafe Window CreateWindow(ElementalApplication application, in WindowOptions options)
    {
        fixed (byte* TitlePinned = options.Title)
        {
            var optionsUnsafe = new WindowOptionsUnsafe();
            optionsUnsafe.Title = TitlePinned;
            optionsUnsafe.Width = options.Width;
            optionsUnsafe.Height = options.Height;
            optionsUnsafe.WindowState = options.WindowState;

            return ApplicationServiceInterop.CreateWindow(application, optionsUnsafe);
        }
    }

    public void FreeWindow(Window window)
    {
        ApplicationServiceInterop.FreeWindow(window);
    }

    public WindowSize GetWindowRenderSize(Window window)
    {
        return ApplicationServiceInterop.GetWindowRenderSize(window);
    }

    public void SetWindowTitle(Window window, ReadOnlySpan<byte> title)
    {
        ApplicationServiceInterop.SetWindowTitle(window, title);
    }

    public void SetWindowTitle(Window window, string title)
    {
        ApplicationServiceInterop.SetWindowTitle(window, Encoding.UTF8.GetBytes(title));
    }

    public void SetWindowState(Window window, WindowState windowState)
    {
        ApplicationServiceInterop.SetWindowState(window, windowState);
    }

}
