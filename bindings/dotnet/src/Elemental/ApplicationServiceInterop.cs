[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]

namespace Elemental;

internal static partial class ApplicationServiceInterop
{
    [LibraryImport("Elemental.Native", EntryPoint = "ElemConfigureLogHandler")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void ConfigureLogHandler(LogHandler logHandler);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetSystemInfo")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial SystemInfoUnsafe GetSystemInfo();

    [LibraryImport("Elemental.Native", EntryPoint = "ElemRunApplication")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial int RunApplication(in RunApplicationParametersUnsafe parameters);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemExitApplication")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void ExitApplication();

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateWindow")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial Window CreateWindow(in WindowOptionsUnsafe options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreeWindow")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreeWindow(Window window);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetWindowRenderSize")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial WindowSize GetWindowRenderSize(Window window);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemSetWindowTitle")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void SetWindowTitle(Window window, ReadOnlySpan<byte> title);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemSetWindowState")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void SetWindowState(Window window, WindowState windowState);

}
