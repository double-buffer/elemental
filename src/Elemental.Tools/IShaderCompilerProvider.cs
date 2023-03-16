namespace Elemental.Tools;

internal interface IShaderCompilerProvider
{
    ShaderLanguage ShaderLanguage { get; }
    ReadOnlySpan<ShaderLanguage> TargetShaderLanguages { get; }

    bool IsCompilerInstalled();
}