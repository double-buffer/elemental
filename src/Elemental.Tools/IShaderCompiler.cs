namespace Elemental.Tools;

/// <summary>
/// Manages the compilation of shader source code to a target API language.
/// </summary>
public interface IShaderCompiler
{
    /// <summary>
    /// Checks if the compiler can compile the specified <see cref="ShaderLanguage" /> to the
    /// specified <see cref="ToolsGraphicsApi" />.
    /// </summary>
    /// <param name="shaderLanguage">Source shader language.</param>
    /// <param name="graphicsApi">Target graphics API.</param>
    /// <returns>True if the compiler can do the compilation; Otherwise, False.</returns>
    bool CanCompileShader(ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi);

    /// <summary>
    /// Compiles the shader.
    /// </summary>
    /// <param name="shaderCode">Shader source code.</param>
    /// <returns>Shader Compiler results object.</returns>
    ShaderCompilerResult CompileShader(ReadOnlySpan<char> shaderCode);
}