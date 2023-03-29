namespace Elemental.Tools;

/// <summary>
/// Represents a shader compiler that can be used to compile shaders for a specific graphics API.
/// </summary>
[PlatformService(InitMethod = nameof(PlatformServiceInterop.Native_InitShaderCompiler), DisposeMethod = nameof(PlatformServiceInterop.Native_FreeShaderCompiler))] 
public interface IShaderCompiler
{
    /// <summary>
    /// Determines whether the shader compiler can compile shaders for the specified shader language and graphics API.
    /// </summary>
    /// <param name="shaderLanguage">The shader language to check.</param>
    /// <param name="graphicsApi">The graphics API to check.</param>
    /// <returns><c>true</c> if the shader compiler can compile shaders for the specified shader language and graphics API; otherwise, <c>false</c>.</returns>
    bool CanCompileShader(ShaderLanguage shaderLanguage, GraphicsApi graphicsApi);

    /// <summary>
    /// Compiles the specified shader code into a shader object.
    /// </summary>
    /// <param name="shaderCode">The source code of the shader to compile.</param>
    /// <param name="shaderStage">The stage of the shader to compile.</param>
    /// <param name="entryPoint">The name of the entry point function for the shader.</param>
    /// <param name="shaderLanguage">The language of the shader to compile.</param>
    /// <param name="graphicsApi">The graphics API to compile the shader for.</param>
    /// <param name="options">The compilation options to use for the shader.</param>
    /// <returns>A <see cref="ShaderCompilerResult"/> object representing the compiled shader.</returns>
    ShaderCompilerResult CompileShader(string shaderCode, ShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, in ShaderCompilationOptions options = default);
}