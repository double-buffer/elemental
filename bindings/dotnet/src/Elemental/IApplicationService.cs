namespace Elemental;

public interface IApplicationService
{
    void ConfigureLogHandler(LogHandler logHandler);
    Application CreateApplication(ReadOnlySpan<byte> applicationName);
    void RunApplication(Application application, RunHandler runHandler);
    void FreeApplication(Application application);
}
