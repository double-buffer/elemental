namespace Elemental;

public delegate bool RunHandlerDelegate(NativeApplicationStatus status);

[PlatformService]
public interface INativeApplicationService
{
    NativeApplication CreateApplication(string applicationName);
    void RunApplication(NativeApplication application, RunHandlerDelegate runHandler);

    NativeWindow CreateWindow(NativeApplication application, NativeWindowDescription description);
}
