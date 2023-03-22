[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]

namespace Elemental;

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Tools.Native")]
    internal static partial void Native_Hello();
}