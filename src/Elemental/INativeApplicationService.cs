namespace Elemental;

/// <summary>
/// Manages the application lifecycle and windows.
/// </summary>
[PlatformService]
public interface INativeApplicationService
{
    /// <summary>
    /// Creates a new native application.
    /// </summary>
    /// <param name="applicationName">Name of the application.</param>
    /// <returns>Application handle.</returns>
    NativeApplication CreateApplication(string applicationName);

    /// <summary>
    /// Executes the main loop for the specified application. This method returns
    /// when the application is closed. 
    /// </summary>
    /// <param name="application">Application used to run the main loop.</param>
    /// <param name="runHandler">Handler called at each iterations of the loop.</param>
    void RunApplication(NativeApplication application, RunHandler runHandler);

    /// <summary>
    /// Creates a new native window.
    /// </summary>
    /// <param name="application">Application that owns the window.</param>
    /// <param name="description">Description of the window to create.</param>
    /// <returns>Window handle.</returns>
    NativeWindow CreateWindow2(NativeApplication application, NativeWindowDescription description);

    /// <summary>
    /// Gets the available render size of the specified window.
    /// </summary>
    /// <param name="window">Window handle.</param>
    /// <returns>Informations about the size of the window.</returns>
    NativeWindowSize GetWindowRenderSize(NativeWindow window);

    /// <summary>
    /// Sets the title of the window.
    /// </summary>
    /// <param name="window">Window handle.</param>
    /// <param name="title">Title of the window.</param>
    void SetWindowTitle(NativeWindow window, string title);
}

/// <summary>
/// Delegate used by the RunApplication method. It is called at each iterations
/// of the main loop.
/// </summary>
/// <param name="status">Current status of the application.</param>
/// <returns>True if the main loop should continue; False otherwise.</returns>
public delegate bool RunHandler(NativeApplicationStatus status);