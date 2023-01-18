using System.Runtime.InteropServices;

namespace Elemental;

public interface INativeApplicationService
{
    NativeApplication CreateApplication(string applicationName);
    void RunApplication(NativeApplication application, Func<NativeApplicationStatus, bool> runHandler);
}

public static partial class TestInterop
{
    [LibraryImport("TestInterop", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void HelloWorld(string test, Func<NativeApplicationStatus, bool> runHandler);
}