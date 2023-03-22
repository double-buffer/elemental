namespace Elemental.Tools;

/// <summary>
/// Manages the compilation of shader source code to a target API language.
/// </summary>
[PlatformService] 
public interface IShaderCompiler
{
    void Hello();

    /// <summary>
    /// Checks if the compiler can compile the specified <see cref="ShaderLanguage" /> to the
    /// specified <see cref="ToolsGraphicsApi" />.
    /// </summary>
    /// <param name="shaderLanguage">Source shader language.</param>
    /// <param name="graphicsApi">Target graphics API.</param>
    /// <returns>True if the compiler can do the compilation; Otherwise, False.</returns>
    [PlatformMethodIgnore] 
    bool CanCompileShader(ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi);

    /// <summary>
    /// Compiles the shader.
    /// </summary>
    /// <param name="shaderCode">Shader source code.</param>
    /// <param name="shaderStage">Shader stage to compile.</param>
    /// <param name="entryPoint">Entry point of the shader.</param>
    /// <param name="shaderLanguage">Source shader language.</param>
    /// <param name="graphicsApi">Target graphics API.</param>
    /// <returns>Shader Compiler results object.</returns>
    // TODO: Add options for debug code generation 
    [PlatformMethodIgnore] 
    ShaderCompilerResult CompileShader(ReadOnlySpan<char> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi);
}