namespace Elemental;

/// <summary>
/// Defines an interface for Application services.
/// </summary>
public interface IApplicationService
{
    /// <summary>
    /// Configures a custom log handler for processing log messages generated by the application.
    /// </summary>
    /// <param name="logHandler">The function to call when a log message is generated.</param>
    void ConfigureLogHandler(LogHandler logHandler);

    /// <summary>
    /// Retrieves system-related information, such as platform and application path.
    /// </summary>
    /// <returns>A structure containing system information.</returns>
    SystemInfo GetSystemInfo();

    /// <summary>
    /// Starts the execution of an application with specified parameters.
    /// </summary>
    /// <param name="parameters">Configuration and handlers for the application lifecycle.</param>
    /// <returns>Status code indicating success or error.</returns>
    int RunApplication(in RunApplicationParameters parameters);

    /// <summary>
    /// Exits the application, performing necessary cleanup.
    /// </summary>
    void ExitApplication();

    /// <summary>
    /// Creates a window with the specified options or default settings if none are provided.
    /// </summary>
    /// <param name="options">Configuration options for the window; NULL for defaults.</param>
    /// <returns>A handle to the newly created window.</returns>
    Window CreateWindow(in WindowOptions options = default);

    /// <summary>
    /// Releases resources associated with a window.
    /// </summary>
    /// <param name="window">Handle to the window to be freed.</param>
    void FreeWindow(Window window);

    /// <summary>
    /// Gets the render size of a window, accounting for DPI scaling.
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <returns>Render size of the window.</returns>
    WindowSize GetWindowRenderSize(Window window);

    /// <summary>
    /// Sets a window's title.
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="title">New title for the window.</param>
    void SetWindowTitle(Window window, ReadOnlySpan<byte> title);

    /// <summary>
    /// Sets a window's title.
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="title">New title for the window.</param>
    void SetWindowTitle(Window window, string title);

    /// <summary>
    /// Changes the state of a window (e.g., minimize, maximize).
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="windowState">New state for the window.</param>
    void SetWindowState(Window window, WindowState windowState);
}
