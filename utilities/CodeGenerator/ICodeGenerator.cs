namespace Elemental.Tools.CodeGenerator;

public interface ICodeGenerator
{
    void GenerateCode(CppCompilation compilation, string source, string input, string output);
}
