[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]

namespace Elemental;

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_InitNativeApplicationService(in NativeApplicationOptions options = default);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeNativeApplicationService();

    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf16)]
    internal static partial NativeApplication Native_CreateApplication(string applicationName);

    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeApplication(NativeApplication application);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_RunApplication(NativeApplication application, RunHandler runHandler);
    
    [LibraryImport("Elemental.Native")]
    internal static partial NativeWindow Native_CreateWindow(NativeApplication application, in NativeWindowOptions options);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeWindow(NativeWindow window);
    
    [LibraryImport("Elemental.Native")]
    internal static partial NativeWindowSize Native_GetWindowRenderSize(NativeWindow window);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf16)]
    internal static partial void Native_SetWindowTitle(NativeWindow window, string title);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_SetWindowState(NativeWindow window, NativeWindowState windowState);
}