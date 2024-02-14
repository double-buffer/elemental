namespace Elemental;

public interface IApplicationService
{
    void ConfigureLogHandler(LogHandler logHandler);
    ElementalApplication CreateApplication(ReadOnlySpan<byte> applicationName);
    void RunApplication(ElementalApplication application, RunHandler runHandler);
    void FreeApplication(ElementalApplication application);
}
