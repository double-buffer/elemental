namespace Elemental;

/// <summary>
/// Manages the application lifecycle and windows.
/// </summary>
[PlatformService]
public interface INativeApplicationService
{
    /// <summary>
    /// Creates a new <see cref="NativeApplication" />.
    /// </summary>
    /// <param name="applicationName">Name of the application.</param>
    /// <returns>Application handle.</returns>
    NativeApplication CreateApplication(string applicationName);

    /// <summary>
    /// Frees the specified <see cref="NativeApplication" />.
    /// </summary>
    /// <param name="application">Application handle.</param>
    void FreeApplication(NativeApplication application);

    /// <summary>
    /// Executes the main loop for the specified <see cref="NativeApplication" />. This method returns
    /// when the application is closed. 
    /// </summary>
    /// <param name="application">Application used to run the main loop.</param>
    /// <param name="runHandler">Handler called at each iterations of the loop.</param>
    void RunApplication(NativeApplication application, RunHandler runHandler);

    /// <summary>
    /// Creates a new <see cref="NativeWindow" />.
    /// </summary>
    /// <param name="application">Application that owns the window.</param>
    /// <param name="options">Optional parameters used by the graphics device creation.</param>
    /// <returns>Window handle.</returns>
    NativeWindow CreateWindow(NativeApplication application, in NativeWindowOptions options = default);

    /// <summary>
    /// Frees the specified <see cref="NativeWindow" />.
    /// </summary>
    /// <param name="window">Window handle.</param>
    void FreeWindow(NativeWindow window);
    
    /// <summary>
    /// Gets the available render size of the specified <see cref="NativeWindow" />.
    /// </summary>
    /// <param name="window">Window handle.</param>
    /// <returns>Informations about the size of the window.</returns>
    NativeWindowSize GetWindowRenderSize(NativeWindow window);

    /// <summary>
    /// Sets the title of the <see cref="NativeWindow" />.
    /// </summary>
    /// <param name="window">Window handle.</param>
    /// <param name="title">Title of the window.</param>
    void SetWindowTitle(NativeWindow window, string title);

    /// <summary>
    /// Sets the state of the <see cref="NativeWindow" />.
    /// </summary>
    /// <param name="window">Window handle.</param>
    /// <param name="windowState">State of the window.</param>
    void SetWindowState(NativeWindow window, NativeWindowState windowState);
}

/// <summary>
/// Delegate used by the RunApplication method. It is called at each iterations
/// of the main loop.
/// </summary>
/// <param name="status">Current status of the application.</param>
/// <returns>True if the main loop should continue; False otherwise.</returns>
public delegate bool RunHandler(NativeApplicationStatus status);