using Elemental.Tools;
[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]

namespace Elemental;

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Tools.Native")]
    internal static partial void Native_InitShaderCompiler();
    
    [LibraryImport("Elemental.Tools.Native")]
    internal static partial void Native_FreeShaderCompiler();

    [LibraryImport("Elemental.Tools.Native")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool Native_CanCompileShader(ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi);
}