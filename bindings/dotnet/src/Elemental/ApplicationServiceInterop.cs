[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]

namespace Elemental;

internal static partial class ApplicationServiceInterop
{
    [LibraryImport("Elemental.Native", EntryPoint = "ElemConfigureLogHandler")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void ConfigureLogHandler(LogHandler logHandler);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateApplication")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial ElementalApplication CreateApplication(ReadOnlySpan<byte> applicationName);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemRunApplication")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void RunApplication(ElementalApplication application, RunHandler runHandler);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreeApplication")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreeApplication(ElementalApplication application);

}
