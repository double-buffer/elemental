using Elemental.Tools;
[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]

namespace Elemental;

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Tools.Native")]
    internal static partial void Native_FreeNativePointer(nint pointer);

    [LibraryImport("Elemental.Tools.Native")]
    internal static partial void Native_InitShaderCompiler(in ShaderCompilerOptions options = default);
    
    [LibraryImport("Elemental.Tools.Native")]
    internal static partial void Native_FreeShaderCompiler();

    [LibraryImport("Elemental.Tools.Native")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool Native_CanCompileShader(ShaderLanguage shaderLanguage, GraphicsApi graphicsApi);
    
    [LibraryImport("Elemental.Tools.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial ShaderCompilerResult Native_CompileShader(string shaderCode, ShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, in ShaderCompilationOptions options);
    
    [LibraryImport("Elemental.Tools.Native")]
    internal unsafe static partial void Native_CompileShaders(ReadOnlySpan<ShaderCompilerInput> inputs, int inputCount, GraphicsApi graphicsApi, in ShaderCompilationOptions options, ShaderCompilerResultMarshaller.ShaderCompilerResultUnmanaged* results, out int resultCount);
}