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
    /// Runs the specified application with the provided run handler.
    /// </summary>
    /// <param name="application">The application to run.</param>
    /// <param name="runHandler">The function to call on each run iteration.</param>
    void RunApplication(ElementalApplication application, RunHandler runHandler);

    /// <summary>
    /// Frees resources associated with the given application.
    /// </summary>
    /// <param name="application">The application to free.</param>
    void FreeApplication(ElementalApplication application);
}
