namespace Elemental.Tools.CodeGenerator;

public partial class DotnetCodeGenerator : ICodeGenerator
{
    [GeneratedRegex("##Module_([^#]+)##")]
    private static partial Regex ModuleNameRegex();

    public void GenerateCode(CppCompilation compilation, string input, string output)
    {
        var currentModuleName = string.Empty;
        var currentModuleFunctions = new List<CppFunction>();
        var typeDictionary = new Dictionary<string, CppType>();

        foreach (var function in compilation.Functions)
        {
            if (Path.GetFileName(function.SourceFile) != "Elemental.h" || function.Name == "ElemConsoleLogHandler")
            {
                continue;
            }

            var moduleName = currentModuleName;

            if (function.Comment != null)
            {
                var result = ModuleNameRegex().Match(function.Comment.ToString());

                if (result.Success)
                {
                    currentModuleName = result.Groups[1].Value;
                    Console.WriteLine($"Module: {result.Groups[1].Value}");
                }

                Console.WriteLine($"Test: {function.Comment} {function.Name}");
            }

            if (!string.IsNullOrEmpty(moduleName) && moduleName != currentModuleName)
            {
                GenerateInterface(moduleName, Path.Combine(output, "src", "Elemental"), compilation, currentModuleFunctions, typeDictionary);
                currentModuleFunctions.Clear();
            }

            currentModuleFunctions.Add(function);
        }

        if (!string.IsNullOrEmpty(currentModuleName))
        {
            GenerateInterface(currentModuleName, Path.Combine(output, "src", "Elemental"), compilation, currentModuleFunctions, typeDictionary);
            currentModuleFunctions.Clear();
        }

        foreach (var typeToGenerate in typeDictionary)
        {
            GenerateType(typeToGenerate.Value, Path.Combine(output, "src", "Elemental"), compilation); 
        }
    }
    
    private void GenerateInterface(string name, string outputPath, CppCompilation compilation, IList<CppFunction> functions, Dictionary<string, CppType> typeDictionary)
    {
        var interfaceName = $"I{name}Service";

        Console.WriteLine(outputPath);
        Console.WriteLine($"Creating interface for '{name}'...");

        var stringBuilder = new StringBuilder();
        stringBuilder.AppendLine("namespace Elemental;");
        stringBuilder.AppendLine();

        stringBuilder.AppendLine($"public interface {interfaceName}");
        stringBuilder.AppendLine("{");

        foreach (var function in functions)
        {
            var functionName = function.Name.Replace("Elem", string.Empty);

            Indent(stringBuilder);
            stringBuilder.Append($"{MapType(typeDictionary, function.ReturnType)} {functionName}(");
            var firstParameter = true;

            foreach (var parameter in function.Parameters)
            {
                if (!firstParameter)
                {
                    stringBuilder.Append(", ");
                }
                else
                {
                    firstParameter = false;
                }

                stringBuilder.Append($"{MapType(typeDictionary, parameter.Type)} {parameter.Name}");
            }

            stringBuilder.AppendLine(");");
        }

        stringBuilder.AppendLine("}");

        File.WriteAllText(Path.Combine(outputPath, $"{interfaceName}.cs"), stringBuilder.ToString());
    }

    private void GenerateType(CppType type, string outputPath, CppCompilation compilation)
    {
        var canonicalType = type.GetCanonicalType();
        var typeName = MapType(null, type);
        Console.WriteLine($"Generate type {typeName}...");

        var stringBuilder = new StringBuilder();

        stringBuilder.AppendLine("namespace Elemental;");
        stringBuilder.AppendLine();

        if (type.TypeKind == CppTypeKind.Typedef)
        {
            if (typeName.Contains("Handler"))
            {
                Console.WriteLine(canonicalType.GetDisplayName());
                //stringBuilder.Append("$public delegate {}"
                //"public delegate bool RunHandler(NativeApplicationStatus status);"
            } 
            else
            {
                stringBuilder.AppendLine($"public readonly record struct {typeName}");
                stringBuilder.AppendLine("{");
                
                Indent(stringBuilder);
                stringBuilder.AppendLine($"{MapType(null, canonicalType)} Value {{ get; }}");

                stringBuilder.AppendLine("}");
            }
        }

        File.WriteAllText(Path.Combine(outputPath, $"{typeName}.cs"), stringBuilder.ToString());
    }

    private string MapType(Dictionary<string, CppType>? typeDictionary, CppType type)
    {
        var typeName = type.GetDisplayName().Replace("Elem", string.Empty);

        if (typeDictionary != null)
        {
            var customType = typeName switch
            {
                "const char*" or "void" => false,
                _ => true
            };

            if (!typeDictionary.ContainsKey(typeName) && customType)
            {
                typeDictionary.Add(typeName, type);
            }
        }

        return typeName switch
        {
            "Application" => "ElementalApplication",
            "const char*" => "ReadOnlySpan<byte>",
            "unsigned long long" => "UInt64",
            string value when value.Contains("HandlerPtr") => typeName.Replace("Ptr", string.Empty),
            _ => typeName
        };
    }

    private void Indent(StringBuilder stringBuilder)
    {
        stringBuilder.Append("    ");
    }
} 
