namespace Elemental.Tools;

public interface IShaderCompiler
{
    bool CanCompileShader(ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi);
    void CompileShader(string shaderCode);
}