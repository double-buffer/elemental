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

    /// <summary>
    /// Creates a window for an application with specified options.
    /// </summary>
    /// <param name="application">The associated application instance.</param>
    /// <param name="options">Window creation options; uses defaults if NULL.</param>
    /// <returns>A handle to the created window.</returns>
    public unsafe Window CreateWindow(ElementalApplication application, in WindowOptions options = default)
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

    /// <summary>
    /// Frees resources for a specified window. Call when the window is no longer needed.
    /// </summary>
    /// <param name="window">The window instance to free.</param>
    public void FreeWindow(Window window)
    {
        ApplicationServiceInterop.FreeWindow(window);
    }

    /// <summary>
    /// Gets the render size of a window, accounting for DPI scaling.
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <returns>Render size of the window.</returns>
    public WindowSize GetWindowRenderSize(Window window)
    {
        return ApplicationServiceInterop.GetWindowRenderSize(window);
    }

    /// <summary>
    /// Sets a window's title.
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="title">New title for the window.</param>
    public void SetWindowTitle(Window window, ReadOnlySpan<byte> title)
    {
        ApplicationServiceInterop.SetWindowTitle(window, title);
    }

    /// <summary>
    /// Sets a window's title.
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="title">New title for the window.</param>
    public void SetWindowTitle(Window window, string title)
    {
        ApplicationServiceInterop.SetWindowTitle(window, Encoding.UTF8.GetBytes(title));
    }

    /// <summary>
    /// Changes the state of a window (e.g., minimize, maximize).
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="windowState">New state for the window.</param>
    public void SetWindowState(Window window, WindowState windowState)
    {
        ApplicationServiceInterop.SetWindowState(window, windowState);
    }

}
