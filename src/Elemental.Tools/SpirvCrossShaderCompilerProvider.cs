namespace Elemental.Tools;

internal class SpirvCrossShaderCompilerProvider : IShaderCompilerProvider
{
    public ShaderLanguage ShaderLanguage => ShaderLanguage.Spirv;
    public ReadOnlySpan<ShaderLanguage> TargetShaderLanguages => new[] { ShaderLanguage.Msl };

    public bool IsCompilerInstalled()
    {
        // HACK: Temporary code!
        return true;
    }
    
    public ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        throw new NotImplementedException();
    }
}