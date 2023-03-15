namespace Elemental.Tools;

public class ShaderCompiler : IShaderCompiler
{
    public bool CanCompileShader(ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        return true;
    }

    public void CompileShader(string shaderCode)
    {

    }
}