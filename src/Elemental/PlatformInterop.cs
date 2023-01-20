namespace Elemental;

internal static partial class NativeApplicationServiceInterop
{
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateApplication(string applicationName);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_RunApplication(nint application, RunHandler runHandler);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateWindow(nint application, NativeWindowDescription description);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.UserDirectories)]
    internal static partial NativeWindowSize Native_GetWindowRenderSize(NativeWindow window);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_SetWindowTitle(NativeWindow window, string title);
}