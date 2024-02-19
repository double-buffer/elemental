namespace Elemental;

/// <inheritdoc />
public class ApplicationService : IApplicationService
{
    /// <inheritdoc />
    public void ConfigureLogHandler(LogHandler logHandler)
    {
        ApplicationServiceInterop.ConfigureLogHandler(logHandler);
    }

    /// <inheritdoc />
    public ElementalApplication CreateApplication(ReadOnlySpan<byte> applicationName)
    {
        return ApplicationServiceInterop.CreateApplication(applicationName);
    }

    /// <inheritdoc />
    public void RunApplication(ElementalApplication application, RunHandler runHandler)
    {
        ApplicationServiceInterop.RunApplication(application, runHandler);
    }

    /// <inheritdoc />
    public void FreeApplication(ElementalApplication application)
    {
        ApplicationServiceInterop.FreeApplication(application);
    }

}
