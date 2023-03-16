namespace Elemental.Tools;

internal class DirectXShaderCompilerProvider : IShaderCompilerProvider
{
    public ShaderLanguage ShaderLanguage => ShaderLanguage.Hlsl;
    public ReadOnlySpan<ShaderLanguage> TargetShaderLanguages => new[] { ShaderLanguage.Dxil, ShaderLanguage.Spirv };

    public bool IsCompilerInstalled()
    {
        // HACK: Temporary code!
        return true;
    }
}