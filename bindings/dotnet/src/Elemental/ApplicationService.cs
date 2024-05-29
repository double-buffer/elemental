namespace Elemental;

/// <inheritdoc />
public class ApplicationService : IApplicationService
{
    /// <summary>
    /// Configures a custom log handler for processing log messages generated by the application.
    /// </summary>
    /// <param name="logHandler">The function to call when a log message is generated.</param>
    public void ConfigureLogHandler(LogHandler logHandler)
    {
        ApplicationServiceInterop.ConfigureLogHandler(logHandler);
    }

    /// <summary>
    /// Retrieves system-related information, such as platform and application path.
    /// </summary>
    /// <returns>A structure containing system information.</returns>
    public unsafe SystemInfo GetSystemInfo()
    {
        var resultUnsafe = ApplicationServiceInterop.GetSystemInfo();

        var result = new SystemInfo();
        result.Platform = resultUnsafe.Platform;
        
var ApplicationPathCounter = 0;
var ApplicationPathPointer = (byte*)resultUnsafe.ApplicationPath;

while (ApplicationPathPointer[ApplicationPathCounter] != 0)
{
ApplicationPathCounter++;
}

ApplicationPathCounter++;
        var ApplicationPathSpan = new ReadOnlySpan<byte>(ApplicationPathPointer, ApplicationPathCounter);
        var ApplicationPathArray = new byte[ApplicationPathCounter];
        ApplicationPathSpan.CopyTo(ApplicationPathArray);
        result.ApplicationPath = ApplicationPathArray;
        result.SupportMultiWindows = resultUnsafe.SupportMultiWindows;

        return result;
    }

    /// <summary>
    /// Starts the execution of an application with specified parameters.
    /// </summary>
    /// <param name="parameters">Configuration and handlers for the application lifecycle.</param>
    /// <returns>Status code indicating success or error.</returns>
    public unsafe int RunApplication(in RunApplicationParameters parameters)
    {
        fixed (byte* ApplicationNamePinned = parameters.ApplicationName)
        {
            var parametersUnsafe = new RunApplicationParametersUnsafe();
            parametersUnsafe.ApplicationName = ApplicationNamePinned;
            parametersUnsafe.InitHandler = parameters.InitHandler;
            parametersUnsafe.FreeHandler = parameters.FreeHandler;
            parametersUnsafe.Payload = parameters.Payload;

            return ApplicationServiceInterop.RunApplication(parametersUnsafe);
        }
    }

    /// <summary>
    /// Exits the application, performing necessary cleanup.
    /// </summary>
    /// <param name="exitCode">Exit code of the application.</param>
    public void ExitApplication(int exitCode)
    {
        ApplicationServiceInterop.ExitApplication(exitCode);
    }

    /// <summary>
    /// Creates a window with the specified options or default settings if none are provided.
    /// </summary>
    /// <param name="options">Configuration options for the window; NULL for defaults.</param>
    /// <returns>A handle to the newly created window.</returns>
    public unsafe Window CreateWindow(in WindowOptions options = default)
    {
        fixed (byte* TitlePinned = options.Title)
        {
            var optionsUnsafe = new WindowOptionsUnsafe();
            optionsUnsafe.Title = TitlePinned;
            optionsUnsafe.Width = options.Width;
            optionsUnsafe.Height = options.Height;
            optionsUnsafe.WindowState = options.WindowState;
            optionsUnsafe.IsCursorHidden = options.IsCursorHidden;

            return ApplicationServiceInterop.CreateWindow(optionsUnsafe);
        }
    }

    /// <summary>
    /// Releases resources associated with a window.
    /// </summary>
    /// <param name="window">Handle to the window to be freed.</param>
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
    /// Changes the state of a window (e.g., minimize, maximize).
    /// </summary>
    /// <param name="window">The window instance.</param>
    /// <param name="windowState">New state for the window.</param>
    public void SetWindowState(Window window, WindowState windowState)
    {
        ApplicationServiceInterop.SetWindowState(window, windowState);
    }

    /// <summary>
    /// TODO: Comments
///TODO: Make sure the coordinates are consistent accross all platforms
    /// </summary>
    public void ShowWindowCursor(Window window)
    {
        ApplicationServiceInterop.ShowWindowCursor(window);
    }

    public void HideWindowCursor(Window window)
    {
        ApplicationServiceInterop.HideWindowCursor(window);
    }

    public WindowCursorPosition GetWindowCursorPosition(Window window)
    {
        return ApplicationServiceInterop.GetWindowCursorPosition(window);
    }

}
