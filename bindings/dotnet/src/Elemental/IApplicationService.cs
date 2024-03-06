namespace Elemental;

/// <summary>
/// Defines an interface for Application services.
/// </summary>
public interface IApplicationService
{
    /// <summary>
    /// Configures a custom log handler for the application.
    /// </summary>
    /// <param name="logHandler">The log handler function to be used.</param>
    void ConfigureLogHandler(LogHandler logHandler);

    /// <summary>
    /// Creates a new application instance.
    /// </summary>
    /// <param name="applicationName">The name of the application.</param>
    /// <returns>Returns an application handle.</returns>
    ElementalApplication CreateApplication(ReadOnlySpan<byte> applicationName);

    /// <summary>
    /// Creates a new application instance.
    /// </summary>
    /// <param name="applicationName">The name of the application.</param>
    /// <returns>Returns an application handle.</returns>
    ElementalApplication CreateApplication(string applicationName);

    /// <summary>
    /// Frees resources associated with the given application.
    /// </summary>
    /// <param name="application">The application to free.</param>
    void FreeApplication(ElementalApplication application);

    /// <summary>
    /// Runs the specified application with the provided run handler.
    /// </summary>
    /// <param name="application">The application to run.</param>
    /// <param name="runHandler">The function to call on each run iteration.</param>
    void RunApplication(ElementalApplication application, RunHandler runHandler);

    /// <summary>
    /// Creates a window for an application with specified options.
    /// </summary>
    /// <summary>
    /// 
    /// </summary>
    /// <param name="application">The associated application instance.</param>
    /// <param name="options">Window creation options; uses defaults if NULL.</param>
    /// <returns>A handle to the created window.</returns>
    Window CreateWindow(ElementalApplication application, in WindowOptions options);

    /// <summary>
    /// Frees resources for a specified window. Call when the window is no longer needed.
    /// </summary>
    /// <summary>
    /// 
    /// </summary>
    /// <param name="window">The window instance to free.</param>
    void FreeWindow(Window window);

    /// <summary>
    /// Gets the render size of a window, accounting for DPI scaling.
    /// </summary>
    /// <summary>
    /// 
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <returns>Render size of the window.</returns>
    WindowSize GetWindowRenderSize(Window window);

    /// <summary>
    /// Sets a window's title.
    /// </summary>
    /// <summary>
    /// 
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="title">New title for the window.</param>
    void SetWindowTitle(Window window, ReadOnlySpan<byte> title);

    /// <summary>
    /// Sets a window's title.
    /// </summary>
    /// <summary>
    /// 
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="title">New title for the window.</param>
    void SetWindowTitle(Window window, string title);

    /// <summary>
    /// Changes the state of a window (e.g., minimize, maximize).
    /// </summary>
    /// <summary>
    /// 
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="windowState">New state for the window.</param>
    void SetWindowState(Window window, WindowState windowState);
}
