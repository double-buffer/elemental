namespace Elemental.Tools;

internal interface IShaderCompilerProvider
{
    ShaderLanguage ShaderLanguage { get; }
    ReadOnlySpan<ShaderLanguage> TargetShaderLanguages { get; }

    bool IsCompilerInstalled();
    ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi);
}