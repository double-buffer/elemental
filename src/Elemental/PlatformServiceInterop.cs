[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]

namespace Elemental;

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateApplication(string applicationName);

    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_DeleteApplication(nint application);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_RunApplication(nint application, RunHandler runHandler);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateWindow(nint application, NativeWindowDescription description);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_DeleteWindow(nint window);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial NativeWindowSize Native_GetWindowRenderSize(NativeWindow window);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_SetWindowTitle(NativeWindow window, string title);
}

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics);

    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_DeleteGraphicsDevice(nint graphicsDevice);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(nint graphicsDevice);
}