namespace Elemental.Tools.CodeGenerator;

public partial class DotnetCodeGenerator : ICodeGenerator
{
    [GeneratedRegex(@"##Module_([^#]+)##")]
    private static partial Regex ModuleNameRegex();

    [GeneratedRegex(@"^(\w+\s*\*?)\s*\(\*\)\((.*?)\)\*")]
    private static partial Regex FunctionPointerRegex();

    [GeneratedRegex(@"((?:\bconst\b\s+)?[\w&]+(?:\s*\*+\s*|\s+))(?:(\w+)\s*)\b")]
    private static partial Regex FunctionPointerParameterRegex();

    [GeneratedRegex(@"(?:const\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\*")]
    private static partial Regex ConstStructPointerRegex();

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

    public void GenerateCode(CppCompilation compilation, string source, string input, string output)
    {
        var currentModuleName = string.Empty;
        var currentModuleFunctions = new List<CppFunction>();

        var modules = new List<(int Offset, string Name)>();
        var moduleFunctions = new Dictionary<string, List<CppFunction>>();
        var results = ModuleNameRegex().Matches(source);

        foreach (Match result in results)
        {
            modules.Add((result.Index, result.Groups[1].Value));
        }

        modules.Add((source.Length, "End"));

        foreach (var function in compilation.Functions)
        {
            if (Path.GetFileName(function.SourceFile) != "Elemental.h" || function.Name == "ElemConsoleLogHandler")
            {
                continue;
            }

            var moduleName = string.Empty;

            foreach (var module in modules)
            {
                if (function.Span.Start.Offset < module.Offset)
                {
                    if (!moduleFunctions.ContainsKey(moduleName))
                    {
                        moduleFunctions.Add(moduleName, new List<CppFunction>());
                    }

                    moduleFunctions[moduleName].Add(function);
                    break;
                }

                moduleName = module.Name;
            }
        }

        foreach (var module in moduleFunctions)
        {
            var modulePath = GetModulePath(output, module.Key);

            GenerateInterface(module.Key, modulePath, compilation, module.Value);
            GenerateService(module.Key, modulePath, compilation, module.Value);
            GenerateInterop(module.Key, modulePath, compilation, module.Value);
        }

        GenerateTypes(compilation, compilation.Enums, output, modules);
        GenerateTypes(compilation, compilation.Typedefs, output, modules);
        GenerateTypes(compilation, compilation.Classes, output, modules);
    }

    private void GenerateTypes(CppCompilation compilation, IEnumerable<CppType> types, string output, List<(int Offset, string Name)> modules)
    {
        foreach (var type in types)
        {
            if (Path.GetFileName(type.SourceFile) != "Elemental.h" || type.GetDisplayName() == "ElemHandle")
            {
                continue;
            }

            var moduleName = string.Empty;

            foreach (var module in modules)
            {
                if (type.Span.Start.Offset < module.Offset)
                {
                    var modulePath = GetModulePath(output, moduleName);
                    GenerateType(moduleName, type, modulePath, compilation);
                    break;
                }

                moduleName = module.Name;
            }
        }
    }

    private void GenerateInterface(string name, string outputPath, CppCompilation compilation, IList<CppFunction> functions)
    {
        var interfaceName = $"I{name}Service";

        Console.WriteLine($"Creating interface for module '{name}'...");

        var stringBuilder = new StringBuilder();
        stringBuilder.AppendLine($"namespace {GetNamespace(name)};");
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

            var containsStringParameters = GenerateModuleFunctionDefinition(stringBuilder, function, generateStringParameters: false);
            stringBuilder.AppendLine(";");

            if (containsStringParameters)
            {
                stringBuilder.AppendLine();
                GenerateModuleFunctionDefinition(stringBuilder, function, generateStringParameters: true);
                stringBuilder.AppendLine(";");
            }
        }

        stringBuilder.AppendLine("}");

        File.WriteAllText(Path.Combine(outputPath, $"{interfaceName}.cs"), stringBuilder.ToString());
    }

    private void GenerateService(string name, string outputPath, CppCompilation compilation, IList<CppFunction> functions)
    {
        var serviceName = $"{name}Service";

        Console.WriteLine($"Creating service for module '{name}'...");

        var stringBuilder = new StringBuilder();
        stringBuilder.AppendLine($"namespace {GetNamespace(name)};");
        stringBuilder.AppendLine();

        stringBuilder.AppendLine("/// <inheritdoc />");
        stringBuilder.AppendLine($"public class {serviceName} : I{serviceName}");
        stringBuilder.AppendLine("{");

        foreach (var function in functions)
        {
            var containsStringParameters = GenerateServiceFunction(serviceName, stringBuilder, function, generateStringParameters: false);
            
            if (containsStringParameters)
            {
                GenerateServiceFunction(serviceName, stringBuilder, function, generateStringParameters: true);
            }
        }

        stringBuilder.AppendLine("}");

        File.WriteAllText(Path.Combine(outputPath, $"{serviceName}.cs"), stringBuilder.ToString());
    }

    private bool GenerateServiceFunction(string serviceName, StringBuilder stringBuilder, CppFunction function, bool generateStringParameters)
    {
        var functionName = function.Name.Replace("Elem", string.Empty);
        var firstParameter = true;

        var customMarshallerTypes = new Dictionary<string, CppType>();

        foreach (var parameter in function.Parameters)
        {
            if (parameter.Type is CppPointerType pointerType)
            {
                var qualifiedType = pointerType.ElementType as CppQualifiedType;

                if (qualifiedType == null)
                {
                    continue;
                }

                var structType = qualifiedType.ElementType as CppClass;
                
                if (structType != null)
                {
                    if (NeedCustomMarshaller(structType))
                    {
                        customMarshallerTypes.Add(parameter.Name, structType);
                        continue;
                    }
                }
            }
        }

        var containsStringParameters = GenerateModuleFunctionDefinition(stringBuilder, function, generateStringParameters, appendPublic: true, appendUnsafe: customMarshallerTypes.Count > 0);
        stringBuilder.AppendLine();

        Indent(stringBuilder);
        stringBuilder.AppendLine("{");

        var currentTabLevel = 2;

        foreach (var parameterToMarshal in customMarshallerTypes)
        {
            if (parameterToMarshal.Value is CppClass structType)
            {
                foreach (var field in structType.Fields)
                {
                    if (NeedCustomMarshaller(field.Type))
                    {
                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"fixed ({MapType(field.Type.GetDisplayName(), isUnsafe: true)} {field.Name}Pinned = {parameterToMarshal.Key}.{field.Name})");

                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine("{");

                        currentTabLevel++;
                    }
                }
            }
        }

        foreach (var parameterToMarshal in customMarshallerTypes)
        {
            Indent(stringBuilder, currentTabLevel);
            stringBuilder.AppendLine($"var {parameterToMarshal.Key}Unsafe = new {MapType(parameterToMarshal.Value.GetDisplayName())}Unsafe();");

            if (parameterToMarshal.Value is CppClass structType)
            {
                foreach (var field in structType.Fields)
                {
                    if (NeedCustomMarshaller(field.Type))
                    {
                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"{parameterToMarshal.Key}Unsafe.{field.Name} = {field.Name}Pinned;");
                    }
                    else
                    {
                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"{parameterToMarshal.Key}Unsafe.{field.Name} = {parameterToMarshal.Key}.{field.Name};");
                    }
                }
            }

            stringBuilder.AppendLine();
        }

        Indent(stringBuilder, currentTabLevel);

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

            if (parameter.Type.GetDisplayName() == "const char*" && generateStringParameters)
            {
                stringBuilder.Append($"Encoding.UTF8.GetBytes({parameter.Name})");
            }
            else if (customMarshallerTypes.ContainsKey(parameter.Name))
            {
                stringBuilder.Append($"{parameter.Name}Unsafe");
            }
            else
            {
                stringBuilder.Append($"{parameter.Name}");
            }
        }

        stringBuilder.AppendLine(");");

        foreach (var parameterToMarshal in customMarshallerTypes)
        {
            if (parameterToMarshal.Value is CppClass structType)
            {
                foreach (var field in structType.Fields)
                {
                    if (NeedCustomMarshaller(field.Type))
                    {
                        currentTabLevel--;

                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine("}");
                    }
                }
            }
        }

        Indent(stringBuilder);
        stringBuilder.AppendLine("}");
        stringBuilder.AppendLine();

        return containsStringParameters;
    }

    private void GenerateInterop(string name, string outputPath, CppCompilation compilation, IList<CppFunction> functions)
    {
        var serviceName = $"{name}ServiceInterop";

        Console.WriteLine($"Creating interop for module '{name}'...");

        var stringBuilder = new StringBuilder();

        if (name == "Application")
        {
            stringBuilder.AppendLine("[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory)]");
            stringBuilder.AppendLine();
        }

        stringBuilder.AppendLine($"namespace {GetNamespace(name)};");
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
            stringBuilder.Append($"internal static partial {MapType(function.ReturnType)} {functionName}(");
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

                if (parameter.Type is CppPointerType pointerType)
                {
                    var qualifiedType = pointerType.ElementType as CppQualifiedType;

                    if (qualifiedType == null)
                    {
                        continue;
                    }

                    var structType = qualifiedType.ElementType as CppClass;
                    
                    if (structType != null)
                    {
                        if (NeedCustomMarshaller(structType))
                        {
                            stringBuilder.Append($"{MapType(parameter.Type)}Unsafe {parameter.Name}");
                            continue;
                        }
                    }
                }

                stringBuilder.Append($"{MapType(parameter.Type)} {parameter.Name}");
            }

            stringBuilder.AppendLine(");");
            stringBuilder.AppendLine();
        }

        stringBuilder.AppendLine("}");

        File.WriteAllText(Path.Combine(outputPath, $"{serviceName}.cs"), stringBuilder.ToString());
    }

    private void GenerateType(string moduleName, CppType type, string outputPath, CppCompilation compilation)
    {
        var canonicalType = type.GetCanonicalType();
        var typeName = MapType(type.GetDisplayName());
        Console.WriteLine($"Generating type {typeName}...");

        var stringBuilder = new StringBuilder();

        stringBuilder.AppendLine($"namespace {GetNamespace(moduleName)};");
        stringBuilder.AppendLine();

        if (type.TypeKind == CppTypeKind.Typedef)
        {
            var typeDefType = (CppTypedef)type;

            if (typeName.Contains("Handler"))
            {
                GenerateDelegate(canonicalType, typeName, stringBuilder, typeDefType);
            }
            else
            {
                if (typeDefType.Comment != null)
                {
                    GenerateComment(stringBuilder, typeDefType.Comment, 0);
                }
                
                stringBuilder.AppendLine($"public readonly record struct {typeName} : IDisposable");
                stringBuilder.AppendLine("{");
                
                Indent(stringBuilder);
                stringBuilder.AppendLine($"private {MapType(canonicalType)} Value {{ get; }}");

                stringBuilder.AppendLine();
                Indent(stringBuilder);
                stringBuilder.AppendLine("///<summary>");
                Indent(stringBuilder);
                stringBuilder.AppendLine("/// Disposes the handler.");
                Indent(stringBuilder);
                stringBuilder.AppendLine("///</summary>");
                
                Indent(stringBuilder);
                stringBuilder.AppendLine("public void Dispose()");
                Indent(stringBuilder);
                stringBuilder.AppendLine("{");

                Indent(stringBuilder, 2);
                stringBuilder.AppendLine($"{moduleName}ServiceInterop.Free{typeName.Replace("Elemental", string.Empty)}(this);");

                Indent(stringBuilder);
                stringBuilder.AppendLine("}");

                stringBuilder.AppendLine("}");
            }
        }
        else if (type.TypeKind == CppTypeKind.Enum)
        {
            GenerateEnum(type, typeName, stringBuilder);
        }
        else if (type.TypeKind == CppTypeKind.StructOrClass)
        {
            GenerateStruct(type, typeName, stringBuilder);
        }

        File.WriteAllText(Path.Combine(outputPath, $"{typeName}.cs"), stringBuilder.ToString());
    }

    private void GenerateDelegate(CppType canonicalType, string typeName, StringBuilder stringBuilder, CppTypedef typeDefType)
    {
        if (typeDefType.Comment != null)
        {
            GenerateComment(stringBuilder, typeDefType.Comment, 0);
        }

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

    private void GenerateEnum(CppType type, string typeName, StringBuilder stringBuilder)
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

        var firstField = true;

        for (var i = 0; i < enumType.Items.Count; i++)
        { 
            if (!firstField)
            {
                stringBuilder.AppendLine();    
            }
            else
            {
                firstField = false;
            }

            var member = enumType.Items[i];

            if (member.Comment != null)
            {
                GenerateComment(stringBuilder, member.Comment, 1);
            }

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

    private bool NeedCustomMarshaller(CppClass structType)
    {
        foreach (var field in structType.Fields)
        {
            if (NeedCustomMarshaller(field.Type))
            {
                return true;
            }
        }

        return false;
    }

    private bool NeedCustomMarshaller(CppType type)
    {
        if (MapType(type).Contains("ReadOnlySpan"))
        {
            return true;
        }

        return false;
    }

    private void GenerateStruct(CppType type, string typeName, StringBuilder stringBuilder)
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

        var needsCustomMarshaller = NeedCustomMarshaller(structType);

        stringBuilder.AppendLine($"public ref struct {typeName}");
        stringBuilder.AppendLine("{");

        var firstField = true;

        foreach (var field in structType.Fields)
        {
            if (!firstField)
            {
                stringBuilder.AppendLine();    
            }
            else
            {
                firstField = false;
            }

            if (field.Comment != null)
            {
                GenerateComment(stringBuilder, field.Comment, 1);
            }

            Indent(stringBuilder);
            stringBuilder.AppendLine($"public {MapType(field.Type.GetDisplayName())} {field.Name} {{ get; set; }}");
        }

        stringBuilder.AppendLine("}");

        if (needsCustomMarshaller)
        {
            stringBuilder.AppendLine();
            stringBuilder.AppendLine($"internal unsafe struct {typeName}Unsafe");
            stringBuilder.AppendLine("{");

            firstField = true;
            var unsafeFields = new List<CppField>();

            foreach (var field in structType.Fields)
            {
                if (!firstField)
                {
                    stringBuilder.AppendLine();    
                }
                else
                {
                    firstField = false;
                }

                Indent(stringBuilder);
                stringBuilder.AppendLine($"public {MapType(field.Type.GetDisplayName(), isUnsafe: true)} {field.Name} {{ get; set; }}");

                if (field.Type.GetDisplayName().Contains("const char*"))
                {
                    unsafeFields.Add(field);
                }
            }
            
            stringBuilder.AppendLine("}");
        }
    }

    private bool GenerateModuleFunctionDefinition(StringBuilder stringBuilder, CppFunction function, bool generateStringParameters, bool appendPublic = false, bool appendUnsafe = false)
    {
        var containsStringParameters = false;
        var functionName = function.Name.Replace("Elem", string.Empty);

        if (function.Comment != null)
        {
            GenerateComment(stringBuilder, function.Comment, 1);
        }

        Indent(stringBuilder);
        
        if (appendPublic)
        {
            stringBuilder.Append("public ");
        }

        if (appendUnsafe)
        {
            stringBuilder.Append("unsafe ");
        }

        stringBuilder.Append($"{MapType(function.ReturnType)} {functionName}(");
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

            var parameterType = MapType(parameter.Type);

            if (parameter.Type.GetDisplayName() == "const char*")
            {
                containsStringParameters = true;

                if (generateStringParameters)
                {
                    parameterType = "string";
                }
            }

            stringBuilder.Append($"{parameterType} {parameter.Name}");

            if (parameter.Name == "options")
            {
                stringBuilder.Append(" = default");
            }
        }

        stringBuilder.Append(")");
        return containsStringParameters;
    }

    private void GenerateComment(StringBuilder stringBuilder, CppComment rootComment, int indentLevel)
    {
        foreach (var comment in rootComment.Children)
        {
            if (comment.ToString().Contains("##") || comment.ToString().Trim() == string.Empty)
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

    private string GetNamespace(string moduleName)
    {
        if (moduleName == "Application")
        {
            return "Elemental";
        }

        return $"Elemental.{moduleName}";
    }

    private static string GetModulePath(string output, string moduleName)
    {
        var modulePath = Path.Combine(output, "src", "Elemental");

        if (moduleName != "Application")
        {
            modulePath = Path.Combine(modulePath, moduleName);

            if (!Directory.Exists(modulePath))
            {
                Directory.CreateDirectory(modulePath);
            }
        }

        return modulePath;
    }

    private string MapType(CppType type)
    {
        return MapType(type.GetDisplayName());    
    }

    private string MapType(string typeName, bool isUnsafe = false)
    {
        typeName = typeName.Replace("Elem", string.Empty);

        return typeName switch
        {
            "Application" => "ElementalApplication",
            "const char*" => !isUnsafe ? "ReadOnlySpan<byte>" : "byte*",
            "unsigned long long" => "UInt64",
            "unsigned int" => "uint",
            string value when ConstStructPointerRegex().IsMatch(value) => "in " + ConstStructPointerRegex().Match(value).Groups[1].Value,
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