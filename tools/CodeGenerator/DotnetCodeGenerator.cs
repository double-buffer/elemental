namespace Elemental.Tools.CodeGenerator;

public partial class DotnetCodeGenerator : ICodeGenerator
{
    [GeneratedRegex(@"##Module_([^#]+)##")]
    private static partial Regex ModuleNameRegex();

    [GeneratedRegex(@"^(\w+\s*\*?)\s*\(\*\)\((.*?)\)\*")]
    private static partial Regex FunctionPointerRegex();

    [GeneratedRegex(@"((?:\bconst\b\s+)?[\w&]+(?:\s*\*+\s*|\s+))(?:(\w+)\s*)\b")]
    private static partial Regex FunctionPointerParameterRegex();

    private const string HandlerMarshallerTemplate = """
        [CustomMarshaller(typeof(##TYPENAME##), MarshalMode.ManagedToUnmanagedIn, typeof(##TYPENAME##Marshaller))]
        internal static unsafe class ##TYPENAME##Marshaller
        {
            internal sealed unsafe record InterceptorEntry
            {
                public required ##TYPENAME## Callback { get; init; }
                public required GCHandle Handle { get; init; }
            }

            private static InterceptorEntry? _interceptorEntry;

            private static unsafe void Interceptor(##PARAMETERS_DECLARATION##)
            {
                if (_interceptorEntry == null || function == null || message == null)
                {
                    return;
                }
        ##CUSTOM_BEFORE_CODE##
                _interceptorEntry.Callback(##PARAMETERS_VALUE##);
            }

            public static nint ConvertToUnmanaged(##TYPENAME## managed)
            {
                // TODO: Unallocate handle
                var interceptorDelegate = Interceptor;
                var handle = GCHandle.Alloc(interceptorDelegate);
                var unmanaged = Marshal.GetFunctionPointerForDelegate(interceptorDelegate);

                _interceptorEntry = new InterceptorEntry { Callback = managed, Handle = handle };
                return unmanaged;
            }

            public static void Free(nint _)
            {
            }
        }
        """;
    
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
                }
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
            GenerateService(currentModuleName, Path.Combine(output, "src", "Elemental"), compilation, currentModuleFunctions, typeDictionary);
            GenerateInterop(currentModuleName, Path.Combine(output, "src", "Elemental"), compilation, currentModuleFunctions, typeDictionary);
            currentModuleFunctions.Clear();
        }

        foreach(var enumObject in compilation.Enums)
        {
            if (!typeDictionary.ContainsKey(enumObject.Name))
            {
                typeDictionary.Add(enumObject.Name, enumObject);
            }
        }

        foreach(var structObject in compilation.Classes)
        {
            if (!typeDictionary.ContainsKey(structObject.Name))
            {
                typeDictionary.Add(structObject.Name, structObject);
            }
        }

        foreach (var typeToGenerate in typeDictionary)
        {
            if (Path.GetFileName(typeToGenerate.Value.SourceFile) != "Elemental.h")
            {
                continue;
            }

            GenerateType(typeToGenerate.Value, Path.Combine(output, "src", "Elemental"), compilation); 
        }
    }
    
    private void GenerateInterface(string name, string outputPath, CppCompilation compilation, IList<CppFunction> functions, Dictionary<string, CppType> typeDictionary)
    {
        var interfaceName = $"I{name}Service";

        Console.WriteLine($"Creating interface for module '{name}'...");

        var stringBuilder = new StringBuilder();
        stringBuilder.AppendLine("namespace Elemental;");
        stringBuilder.AppendLine();
                        
        stringBuilder.AppendLine($"/// <summary>");
        stringBuilder.AppendLine($"/// Defines an interface for {name} services.");
        stringBuilder.AppendLine($"/// </summary>");

        stringBuilder.AppendLine($"public interface {interfaceName}");
        stringBuilder.AppendLine("{");

        var firstFunction = true;

        foreach (var function in functions)
        {
            if (!firstFunction)
            {
                stringBuilder.AppendLine();    
            }
            else
            {
                firstFunction = false;
            }

            var functionName = function.Name.Replace("Elem", string.Empty);

            if (function.Comment != null)
            {
                GenerateComment(stringBuilder, function.Comment, 1);
            }

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

    private void GenerateService(string name, string outputPath, CppCompilation compilation, IList<CppFunction> functions, Dictionary<string, CppType> typeDictionary)
    {
        var serviceName = $"{name}Service";

        Console.WriteLine($"Creating service for module '{name}'...");

        var stringBuilder = new StringBuilder();
        stringBuilder.AppendLine("namespace Elemental;");
        stringBuilder.AppendLine();

        stringBuilder.AppendLine("/// <inheritdoc />");
        stringBuilder.AppendLine($"public class {serviceName} : I{serviceName}");
        stringBuilder.AppendLine("{");

        foreach (var function in functions)
        {
            var functionName = function.Name.Replace("Elem", string.Empty);
            
            Indent(stringBuilder);
            stringBuilder.AppendLine("/// <inheritdoc />");
            
            Indent(stringBuilder);
            stringBuilder.Append($"public {MapType(typeDictionary, function.ReturnType)} {functionName}(");
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

            stringBuilder.AppendLine(")");

            Indent(stringBuilder);
            stringBuilder.AppendLine("{");

            Indent(stringBuilder, 2);

            if (function.ReturnType.GetDisplayName() != "void")
            {
                stringBuilder.Append("return ");
            }

            stringBuilder.Append($"{serviceName}Interop.{functionName}(");
            
            firstParameter = true;

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

                stringBuilder.Append($"{parameter.Name}");
            }

            stringBuilder.AppendLine(");");

            Indent(stringBuilder);
            stringBuilder.AppendLine("}");
            stringBuilder.AppendLine();
        }

        stringBuilder.AppendLine("}");

        File.WriteAllText(Path.Combine(outputPath, $"{serviceName}.cs"), stringBuilder.ToString());
    }

    private void GenerateInterop(string name, string outputPath, CppCompilation compilation, IList<CppFunction> functions, Dictionary<string, CppType> typeDictionary)
    {
        var serviceName = $"{name}ServiceInterop";

        Console.WriteLine($"Creating interop for module '{name}'...");

        var stringBuilder = new StringBuilder();
        stringBuilder.AppendLine("[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]");
        stringBuilder.AppendLine();
        stringBuilder.AppendLine("namespace Elemental;");
        stringBuilder.AppendLine();

        stringBuilder.AppendLine($"internal static partial class {serviceName}");
        stringBuilder.AppendLine("{");

        foreach (var function in functions)
        {
            var functionName = function.Name.Replace("Elem", string.Empty);

            Indent(stringBuilder);
            stringBuilder.AppendLine($"[LibraryImport(\"Elemental.Native\", EntryPoint = \"{function.Name}\")]");
            
            Indent(stringBuilder);
            stringBuilder.AppendLine("[UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]");

            Indent(stringBuilder);
            stringBuilder.Append($"internal static partial {MapType(typeDictionary, function.ReturnType)} {functionName}(");
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
            stringBuilder.AppendLine();
        }

        stringBuilder.AppendLine("}");

        File.WriteAllText(Path.Combine(outputPath, $"{serviceName}.cs"), stringBuilder.ToString());
    }

    private void GenerateType(CppType type, string outputPath, CppCompilation compilation)
    {
        var canonicalType = type.GetCanonicalType();
        var typeName = MapType(type.GetDisplayName());
        Console.WriteLine($"Generating type {typeName}...");

        var stringBuilder = new StringBuilder();

        stringBuilder.AppendLine("namespace Elemental;");
        stringBuilder.AppendLine();

        if (type.TypeKind == CppTypeKind.Typedef)
        {
            var typeDefType = (CppTypedef)type;

            if (typeName.Contains("Handler"))
            {
                if (typeDefType.Comment != null)
                {
                    GenerateComment(stringBuilder, typeDefType.Comment, 0);
                }

                Console.WriteLine($"{typeDefType.Comment?.Kind}");
                var needMarshaller = false;
                var result = FunctionPointerRegex().Match(canonicalType.GetDisplayName());

                if (result.Success)
                {
                    stringBuilder.Append("##MARSHALLER##");
                    stringBuilder.AppendLine("[UnmanagedFunctionPointer(CallingConvention.Cdecl)]");
                    stringBuilder.Append($"public delegate {result.Groups[1].Value.Trim()} {typeName}(");

                    var parameterResults = FunctionPointerParameterRegex().Matches(result.Groups[2].Value);
                    var firstParameter = true;
                    var parametersDeclaration = new StringBuilder();
                    var parametersValue = new StringBuilder();
                    var customBeforeCode = new StringBuilder();

                    foreach (Match parameterResult in parameterResults)
                    {
                        if (!firstParameter)
                        {
                            stringBuilder.Append(", ");
                            parametersDeclaration.Append(", ");
                            parametersValue.Append(", ");
                        }
                        else
                        {
                            firstParameter = false;
                        }

                        var parameterType = MapType(parameterResult.Groups[1].Value.Trim());
                        var parameterName = parameterResult.Groups[2].Value.Trim();
                        
                        if (parameterType.Contains("Span"))
                        {
                            needMarshaller = true;

                            customBeforeCode.AppendLine();
                            customBeforeCode.AppendLine($$"""
                                    var {{parameterName}}Counter = 0;
                                    var {{parameterName}}Pointer = (byte*){{parameterName}};

                                    while ({{parameterName}}Pointer[{{parameterName}}Counter] != 0)
                                    {
                                        {{parameterName}}Counter++;
                                    }

                                    {{parameterName}}Counter++;
                            """);
                        }

                        stringBuilder.Append($"{parameterType} {parameterName}");
                        parametersDeclaration.Append($"{MapTypeToUnmanaged(parameterType)} {parameterName}");
                        parametersValue.Append($"{MapValueToUnmanaged(parameterType, parameterName)}");
                    }

                    stringBuilder.AppendLine(");");

                    if (!needMarshaller)
                    {
                        stringBuilder.Replace("##MARSHALLER##", string.Empty);
                    }
                    else
                    {
                        stringBuilder.Replace("##MARSHALLER##", $"[NativeMarshalling(typeof({typeName}Marshaller))]\n");
                        stringBuilder.AppendLine();
                        stringBuilder.AppendLine(HandlerMarshallerTemplate);
                        stringBuilder.Replace("##TYPENAME##", typeName);
                        stringBuilder.Replace("##PARAMETERS_DECLARATION##", parametersDeclaration.ToString());
                        stringBuilder.Replace("##PARAMETERS_VALUE##", parametersValue.ToString());
                        stringBuilder.Replace("##CUSTOM_BEFORE_CODE##", customBeforeCode.ToString());
                    }
                }
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
        else if (type.TypeKind == CppTypeKind.Enum)
        {
            var enumType = type as CppEnum;
            
            if (enumType == null)
            {
                return;
            }
                
            if (enumType.Comment != null)
            {
                GenerateComment(stringBuilder, enumType.Comment, 0);
            }

            stringBuilder.AppendLine($"public enum {typeName}");
            stringBuilder.AppendLine("{");
            
            for (var i = 0; i < enumType.Items.Count; i++)
            {
                var member = enumType.Items[i];

                Indent(stringBuilder);
                stringBuilder.Append($"{member.Name.Split('_')[1]} = {member.Value}");

                if (i == enumType.Items.Count - 1)
                {
                    stringBuilder.AppendLine();
                }
                else
                {
                    stringBuilder.AppendLine(",");
                }
            }

            stringBuilder.AppendLine("}");
        }
        else if (type.TypeKind == CppTypeKind.StructOrClass)
        {
            var structType = type as CppClass;
            
            if (structType == null)
            {
                return;
            }
            
            if (structType.Comment != null)
            {
                GenerateComment(stringBuilder, structType.Comment, 0);
            }

            stringBuilder.AppendLine($"public record struct {typeName}");
            stringBuilder.AppendLine("{");
            
            foreach (var field in structType.Fields)
            {
                Indent(stringBuilder);
                stringBuilder.AppendLine($"public {MapType(field.Type.GetDisplayName())} {field.Name} {{ get; set; }}");
            }

            stringBuilder.AppendLine("}");
        }

        File.WriteAllText(Path.Combine(outputPath, $"{typeName}.cs"), stringBuilder.ToString());
    }

    private void GenerateComment(StringBuilder stringBuilder, CppComment rootComment, int indentLevel)
    {
        foreach (var comment in rootComment.Children)
        {
            if (comment.ToString().Contains("##"))
            {
                continue;
            }

            if (comment.Kind == CppCommentKind.Paragraph)
            {
                var commentObject = (CppCommentParagraph)comment;

                Indent(stringBuilder, indentLevel);
                stringBuilder.AppendLine($"/// <summary>");

                Indent(stringBuilder, indentLevel);
                stringBuilder.AppendLine($"/// {commentObject.ToString().Trim()}");

                Indent(stringBuilder, indentLevel);
                stringBuilder.AppendLine($"/// </summary>");
            }
            else if (comment.Kind == CppCommentKind.ParamCommand)
            {
                var commentObject = (CppCommentParamCommand)comment;

                Indent(stringBuilder, indentLevel);
                stringBuilder.AppendLine($"/// <param name=\"{commentObject.ParamName}\">{commentObject.ChildrenToString().Trim()}</param>");
            }
            else if (comment.Kind == CppCommentKind.BlockCommand)
            {
                var commentObject = (CppCommentBlockCommand)comment;

                if (commentObject.CommandName == "return")
                {
                    Indent(stringBuilder, indentLevel);
                    stringBuilder.AppendLine($"/// <returns>{commentObject.ChildrenToString().Trim()}</returns>");
                }
            }
        }
    }

    private string MapType(Dictionary<string, CppType>? typeDictionary, CppType type)
    {
        var typeName = type.GetDisplayName();

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

        return MapType(typeName);    
    }

    private string MapType(string typeName)
    {
        typeName = typeName.Replace("Elem", string.Empty);

        return typeName switch
        {
            "Application" => "ElementalApplication",
            "const char*" => "ReadOnlySpan<byte>",
            "unsigned long long" => "UInt64",
            "unsigned int" => "uint",
            string value when value.Contains("HandlerPtr") => typeName.Replace("Ptr", string.Empty),
            _ => typeName
        };
    }

    private string MapTypeToUnmanaged(string typeName)
    {
        return typeName switch
        {
            string value when value.Contains("ReadOnlySpan") => typeName.Replace("ReadOnlySpan<", string.Empty).Replace(">", "*"),
            _ => typeName
        };
    }

    private string MapValueToUnmanaged(string typeName, string valueName)
    {
        return typeName switch
        {
            string value when value.Contains("ReadOnlySpan") => $"new ReadOnlySpan<{typeName.Replace("ReadOnlySpan<", string.Empty).Replace(">", string.Empty)}>({valueName}, {valueName}Counter)",
            _ => valueName
        };
    }

    private void Indent(StringBuilder stringBuilder, int count = 1)
    {
        for (var i = 0; i < count; i++)
        {
            stringBuilder.Append("    ");
        }
    }
} 
