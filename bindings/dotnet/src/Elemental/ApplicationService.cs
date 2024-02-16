namespace Elemental;

public class ApplicationService : IApplicationService
{
    public void ConfigureLogHandler(LogHandler logHandler)
    {
        ApplicationServiceInterop.ConfigureLogHandler(logHandler);
    }

    public ElementalApplication CreateApplication(ReadOnlySpan<byte> applicationName)
    {
        return ApplicationServiceInterop.CreateApplication(applicationName);
    }

    public void RunApplication(ElementalApplication application, RunHandler runHandler)
    {
        ApplicationServiceInterop.RunApplication(application, runHandler);
    }

    public void FreeApplication(ElementalApplication application)
    {
        ApplicationServiceInterop.FreeApplication(application);
    }

}
