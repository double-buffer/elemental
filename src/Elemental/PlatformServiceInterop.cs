[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]

namespace Elemental;

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeNativePointer(nint pointer);

    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateApplication(string applicationName);

    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeApplication(nint application);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_RunApplication(nint application, RunHandler runHandler);
    
    [LibraryImport("Elemental.Native")]
    internal static partial nint Native_CreateWindow(nint application, NativeWindowOptions options);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeWindow(nint window);
    
    [LibraryImport("Elemental.Native")]
    internal static partial NativeWindowSize Native_GetWindowRenderSize(NativeWindow window);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_SetWindowTitle(NativeWindow window, string title);
}