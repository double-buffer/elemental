namespace Elemental.Tools.CodeGenerator;

public interface ICodeGenerator
{
    void GenerateCode(CppCompilation compilation, string input, string output);
}
