namespace Elemental.Tools.CodeGenerator;

public partial class DotnetCodeGenerator : ICodeGenerator
{
    [GeneratedRegex("##Module_([^#]+)##")]
    private static partial Regex ModuleNameRegex();

    public void GenerateCode(CppCompilation compilation, string input, string output)
    {
        Console.WriteLine("DOTNET");

        foreach (var function in compilation.Functions)
        {
            if (Path.GetFileName(function.SourceFile) != "Elemental.h" || function.Name == "ElemConsoleLogHandler")
            {
                continue;
            }

            if (function.Comment != null)
            {
                var result = ModuleNameRegex().Match(function.Comment.ToString());

                if (result.Success)
                {
                    Console.WriteLine($"Module: {result.Groups[1].Value}");
                }

                Console.WriteLine($"Test: {function.Comment} {function.Name}");
            }
        }
    }
} 
