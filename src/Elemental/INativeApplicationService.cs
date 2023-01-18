namespace Elemental;

[PlatformService]
public interface INativeApplicationService
{
    NativeApplication CreateApplication(string applicationName);
    void RunApplication(NativeApplication application, Func<NativeApplicationStatus, bool> runHandler);
}
