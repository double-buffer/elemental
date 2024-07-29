namespace Elemental.Tools.CodeGenerator;

public partial class DotnetCodeGenerator : ICodeGenerator
{
    [GeneratedRegex(@"##Module_([^#]+)##")]
    private static partial Regex ModuleNameRegex();

    [GeneratedRegex(@"^(\w+\s*\*?)\s*\(\*\)\((.*?)\)\s*\*")]
    private static partial Regex FunctionPointerRegex();

    [GeneratedRegex(@"([\w&]+\s+(?:\bconst\b\s+)?(?:\s*\*+\s*|\s+))(?:(\w+)\s*)\b")]
    private static partial Regex FunctionPointerParameterRegex();

    [GeneratedRegex(@"([A-Za-z_][A-Za-z0-9_]*)\s*(?:const\s+)?\*")]
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
                if (_interceptorEntry == null##PARAMETER_CHECKS##)
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
        // TODO: Add support for null handle 
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
            if (Path.GetFileName(type.SourceFile) != "Elemental.h" || type.GetDisplayName() == "ElemHandle" || type.GetDisplayName().Contains("Span"))
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

            var containsStringParameters = GenerateModuleFunctionDefinition(compilation, stringBuilder, function, generateStringParameters: false);
            stringBuilder.AppendLine(";");

            if (containsStringParameters)
            {
                stringBuilder.AppendLine();
                GenerateModuleFunctionDefinition(compilation, stringBuilder, function, generateStringParameters: true);
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
            Console.WriteLine($"Generating Service Function: {function.Name}");
            var containsStringParameters = GenerateServiceFunction(compilation, serviceName, stringBuilder, function, generateStringParameters: false);
            
            if (containsStringParameters)
            {
                GenerateServiceFunction(compilation, serviceName, stringBuilder, function, generateStringParameters: true);
            }
        }

        stringBuilder.AppendLine("}");

        File.WriteAllText(Path.Combine(outputPath, $"{serviceName}.cs"), stringBuilder.ToString());
    }

    private bool GenerateServiceFunction(CppCompilation compilation, string serviceName, StringBuilder stringBuilder, CppFunction function, bool generateStringParameters)
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
                    if (NeedCustomMarshaller(compilation, structType))
                    {
                        customMarshallerTypes.Add(parameter.Name, structType);
                        continue;
                    }
                }
            }
            else if (parameter.Type is CppClass classType)
            {
                if (classType.GetDisplayName().Contains("Span"))
                {
                    customMarshallerTypes.Add(parameter.Name, classType);
                }
            }
        }

        var returnNeedsMarshalling = false;
        CppType? returnTypeMarshalling = null;

        if (function.ReturnType.GetDisplayName() != "void")
        {
            if (function.ReturnType.GetDisplayName().Contains("Span"))
            {
                var returnName = function.ReturnType.GetDisplayName().Replace("Elem", string.Empty);
                var returnTypeString = $"{returnName.Substring(0, returnName.IndexOf("Span"))}";

                var foundType = compilation.FindByName<CppType>($"Elem{returnTypeString}");

                if (foundType != null)
                {
                    returnTypeMarshalling = foundType;
                }

                returnNeedsMarshalling = true;
            }
            else if (NeedCustomMarshaller(compilation, function.ReturnType))
            {
                returnNeedsMarshalling = true;
                returnTypeMarshalling = function.ReturnType;
            }
        }

        var containsStringParameters = GenerateModuleFunctionDefinition(compilation, stringBuilder, function, generateStringParameters, appendPublic: true, appendUnsafe: customMarshallerTypes.Count > 0 || returnNeedsMarshalling);
        stringBuilder.AppendLine();

        Indent(stringBuilder);
        stringBuilder.AppendLine("{");

        var currentTabLevel = 2;

        foreach (var parameterToMarshal in customMarshallerTypes)
        {
            if (parameterToMarshal.Value is CppClass structType)
            {
                var structName = structType.GetDisplayName().Replace("Elem", string.Empty);

                if (structName.Contains("Span"))
                {
                    var pinnedType = $"{structName.Substring(0, structName.IndexOf("Span"))}*";

                    if (pinnedType == "Data*")
                    {
                        pinnedType = "byte*";
                    }

                    Indent(stringBuilder, currentTabLevel);
                    stringBuilder.AppendLine($"fixed ({pinnedType} {parameterToMarshal.Key}Pinned = {parameterToMarshal.Key}.Span)");

                    Indent(stringBuilder, currentTabLevel);
                    stringBuilder.AppendLine("{");

                    currentTabLevel++;
                }

                foreach (var field in structType.Fields)
                {
                    if (FieldNeedCustomMarshaller(compilation, field.Type))
                    {
                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"fixed ({MapType(compilation, field.Type, isUnsafe: true)} {field.Name}Pinned = {parameterToMarshal.Key}.{field.Name})");

                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine("{");

                        currentTabLevel++;
                    }
                }
            }
        }

        foreach (var parameterToMarshal in customMarshallerTypes)
        {
            if (parameterToMarshal.Value is CppClass structType)
            {
                var structName = structType.GetDisplayName().Replace("Elem", string.Empty);
                Indent(stringBuilder, currentTabLevel);

                if (structName.Contains("Span"))
                {
                    var pinnedType = $"{structName.Substring(0, structName.IndexOf("Span"))}";

                    if (pinnedType == "Data")
                    {
                        pinnedType = "byte";
                    }

                    stringBuilder.AppendLine($"var {parameterToMarshal.Key}Unsafe = new SpanUnsafe<{pinnedType}>();");
                }
                else
                {
                    stringBuilder.AppendLine($"var {parameterToMarshal.Key}Unsafe = new {MapType(compilation, parameterToMarshal.Value)}Unsafe();");
                }

                foreach (var field in structType.Fields)
                {
                    Indent(stringBuilder, currentTabLevel);

                    if (FieldNeedCustomMarshaller(compilation, field.Type))
                    {
                        stringBuilder.AppendLine($"{parameterToMarshal.Key}Unsafe.{field.Name} = {field.Name}Pinned;");
                    }
                    else if (field.Name == "Items")
                    {
                        stringBuilder.AppendLine($"{parameterToMarshal.Key}Unsafe.{field.Name} = (nuint){parameterToMarshal.Key}Pinned;");
                    }
                    else
                    {
                        stringBuilder.AppendLine($"{parameterToMarshal.Key}Unsafe.{field.Name} = {parameterToMarshal.Key}.{field.Name};");
                    }
                }

                stringBuilder.AppendLine();
            }
        }

        Indent(stringBuilder, currentTabLevel);

        if (returnTypeMarshalling != null)
        {
            stringBuilder.Append("var resultUnsafe = ");
        }
        else if (function.ReturnType.GetDisplayName() != "void")
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

        if (returnNeedsMarshalling)
        {
            stringBuilder.AppendLine();
            Indent(stringBuilder, currentTabLevel);

            var structType = function.ReturnType as CppClass;
            var srcFieldName = "resultUnsafe";
            var destFieldName = "result";

            if (function.ReturnType.GetDisplayName().Contains("Span") && function.ReturnType.GetDisplayName() != "ElemDataSpan")
            {
                structType = returnTypeMarshalling as CppClass;
                destFieldName = "result[i]";
                srcFieldName = "((GraphicsDeviceInfoUnsafe*)resultUnsafe.Items)[i]";
                        
                stringBuilder.AppendLine($"var result = new {MapType(compilation, returnTypeMarshalling, isReturnValue: true)}[resultUnsafe.Length];");

                Indent(stringBuilder, currentTabLevel);
                stringBuilder.AppendLine($"for (int i = 0; i < resultUnsafe.Length; i++)");

                Indent(stringBuilder, currentTabLevel);
                stringBuilder.AppendLine($"{{");
            }
            else
            {
                stringBuilder.AppendLine($"var result = new {MapType(compilation, function.ReturnType, isReturnValue: true)}();");
            }

            if (structType != null)
            {
                foreach (var field in structType.Fields)
                {
                    if (MapType(compilation, field.Type).Contains("Span"))
                    {
                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine();
                        stringBuilder.AppendLine($$"""
                                var {{field.Name}}Counter = 0;
                                var {{field.Name}}Pointer = (byte*){{srcFieldName}}.{{field.Name}};

                                while ({{field.Name}}Pointer[{{field.Name}}Counter] != 0)
                                {
                                {{field.Name}}Counter++;
                                }

                                {{field.Name}}Counter++;
                                """);

                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"var {field.Name}Span = new ReadOnlySpan<byte>({field.Name}Pointer, {field.Name}Counter);");

                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"var {field.Name}Array = new byte[{field.Name}Counter];");

                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"{field.Name}Span.CopyTo({field.Name}Array);");

                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"{destFieldName}.{field.Name} = {field.Name}Array;");
                    }

                    else
                    {
                        Indent(stringBuilder, currentTabLevel);
                        stringBuilder.AppendLine($"{destFieldName}.{field.Name} = {srcFieldName}.{field.Name};");
                    }
                }
            }
            
            if (function.ReturnType.GetDisplayName().Contains("Span"))
            {
                Indent(stringBuilder, currentTabLevel);
                stringBuilder.AppendLine("}");
            }
            
            stringBuilder.AppendLine();
            Indent(stringBuilder, currentTabLevel);
            stringBuilder.AppendLine($"return result;");
        }
        
        foreach (var parameterToMarshal in customMarshallerTypes)
        {
            if (parameterToMarshal.Value is CppClass structType)
            {
                if (structType.GetDisplayName().Contains("Span"))
                {
                    currentTabLevel--;

                    Indent(stringBuilder, currentTabLevel);
                    stringBuilder.AppendLine("}");
                }

                foreach (var field in structType.Fields)
                {
                    if (FieldNeedCustomMarshaller(compilation, field.Type))
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

            // TODO: Replace with real assembly name
            Indent(stringBuilder);
            stringBuilder.AppendLine($"[LibraryImport(\"Elemental.Native\", EntryPoint = \"{function.Name}\")]");
            
            Indent(stringBuilder);
            stringBuilder.AppendLine("[UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]");

            var returnNeedsMarshalling = NeedCustomMarshaller(compilation, function.ReturnType);

            Indent(stringBuilder);
            stringBuilder.Append($"internal static partial {MapType(compilation, function.ReturnType, forInterop: true, isReturnValue: true)}{(returnNeedsMarshalling ? "Unsafe" : "")} {functionName}(");
          
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
                        if (NeedCustomMarshaller(compilation, structType))
                        {
                            stringBuilder.Append($"{MapType(compilation, parameter.Type)}Unsafe {parameter.Name}");
                            continue;
                        }
                    }
                }

                stringBuilder.Append($"{MapType(compilation, parameter.Type, forInterop: true)} {parameter.Name}");
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
        var typeName = MapType(compilation, type);
        Console.WriteLine($"Generating type {typeName}...");

        var stringBuilder = new StringBuilder();

        stringBuilder.AppendLine($"namespace {GetNamespace(moduleName)};");
        stringBuilder.AppendLine();

        if (type.TypeKind == CppTypeKind.Typedef)
        {
            var typeDefType = (CppTypedef)type;

            if (typeName.Contains("Handler"))
            {
                GenerateDelegate(compilation, canonicalType, typeName, stringBuilder, typeDefType);
            }
            else
            {
                GenerateTypedef(compilation, canonicalType, typeName, stringBuilder, typeDefType, moduleName);
            }
        }
        else if (type.TypeKind == CppTypeKind.Enum)
        {
            GenerateEnum(type, typeName, stringBuilder);
        }
        else if (type.TypeKind == CppTypeKind.StructOrClass)
        {
            GenerateStruct(compilation, type, typeName, stringBuilder);
        }

        File.WriteAllText(Path.Combine(outputPath, $"{typeName}.cs"), stringBuilder.ToString());
    }

    private void GenerateDelegate(CppCompilation compilation, CppType canonicalType, string typeName, StringBuilder stringBuilder, CppTypedef typeDefType)
    {
        if (typeDefType.Comment != null)
        {
            GenerateComment(stringBuilder, typeDefType.Comment, 0);
        }

        Console.WriteLine($"Generate Delegate: {canonicalType.GetDisplayName()}");

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
            var parameterChecks = new StringBuilder();

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

                var parameterType = "ReadOnlySpan<byte>";
                var nativeParameterType = parameterResult.Groups[1].Value.Replace("const *", "").Trim();

                Console.WriteLine($"Type: '{nativeParameterType}'");
                var foundType = compilation.FindByName<CppType>(nativeParameterType);

                if (foundType != null)
                {
                    parameterType = MapType(compilation, foundType);
                }

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
                parameterChecks.Append($" || {parameterName} == null");
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
                stringBuilder.Replace("##PARAMETER_CHECKS##", parameterChecks.ToString());
            }
        }
    }
    
    private void GenerateTypedef(CppCompilation compilation, CppType canonicalType, string typeName, StringBuilder stringBuilder, CppTypedef typeDefType, string moduleName)
    {
        if (typeDefType.Comment != null)
        {
            GenerateComment(stringBuilder, typeDefType.Comment, 0);
        }

        var needToDispose = (compilation.FindByName<CppFunction>($"ElemFree{typeName}") != null);

        stringBuilder.Append($"public readonly record struct {typeName}");

        if (needToDispose)
        {
            stringBuilder.AppendLine(" : IDisposable");
        }
        else
        {
            stringBuilder.AppendLine();
        }

        stringBuilder.AppendLine("{");

        Indent(stringBuilder);
        stringBuilder.AppendLine($"private {MapType(compilation, canonicalType)} Value {{ get; }}");

        if (needToDispose)
        {
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
        }

        stringBuilder.AppendLine("}");
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

    private bool NeedCustomMarshaller(CppCompilation compilation, CppType type)
    {
        CppClass? structType = null; 
            
        if (type is CppClass classType)
        {
            structType = classType;
        }
        else if (type is CppPointerType pointerType)
        {
            var qualifiedType = pointerType.ElementType as CppQualifiedType;

            if (qualifiedType == null)
            {
                return false;
            }

            var classTypeTemp = qualifiedType.ElementType as CppClass;
            
            if (classTypeTemp != null)
            {
                structType = classTypeTemp;
            }
        }

        if (structType == null)
        {
            return false;
        }

        foreach (var field in structType.Fields)
        {
            if (FieldNeedCustomMarshaller(compilation, field.Type))
            {
                return true;
            }
        }

        return false;
    }

    private bool FieldNeedCustomMarshaller(CppCompilation compilation, CppType type)
    {
        var mappedType = MapType(compilation, type);

        if (mappedType.Contains("ReadOnlySpan"))
        {
            return true;
        }

        return false;
    }

    private void GenerateStruct(CppCompilation compilation, CppType type, string typeName, StringBuilder stringBuilder)
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

        var needsCustomMarshaller = NeedCustomMarshaller(compilation, structType);

        if (typeName.Contains("Options") || typeName.Contains("Parameters"))
        {
            stringBuilder.AppendLine($"public ref struct {typeName}");
        }
        else
        {
            stringBuilder.AppendLine($"public ref struct {typeName}");
        }

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
            stringBuilder.AppendLine($"public {MapType(compilation, field.Type)} {field.Name} {{ get; set; }}");
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
                stringBuilder.AppendLine($"public {MapType(compilation, field.Type, isUnsafe: true)} {field.Name} {{ get; set; }}");

                if (field.Type.GetDisplayName().Contains("const char*"))
                {
                    unsafeFields.Add(field);
                }
            }
            
            stringBuilder.AppendLine("}");
            stringBuilder.AppendLine();
        }
    }

    private bool GenerateModuleFunctionDefinition(CppCompilation compilation, StringBuilder stringBuilder, CppFunction function, bool generateStringParameters, bool appendPublic = false, bool appendUnsafe = false)
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

        stringBuilder.Append($"{MapType(compilation, function.ReturnType, isReturnValue: true)} {functionName}(");
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

            var parameterType = MapType(compilation, parameter.Type);

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
                stringBuilder.AppendLine($"/// {commentObject.ToString().Trim().Replace("\n", "\n///")}");

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

    private string GetSpanUnsafe(CppCompilation compilation, string value)
    {
        var needsCustomMarshalling = false;
        var cppType = (compilation.FindByName<CppType>($"Elem{value}"));

        if (cppType != null)
        {
            needsCustomMarshalling = NeedCustomMarshaller(compilation, cppType);
        }

        return $"SpanUnsafe<{value}{(needsCustomMarshalling ? "Unsafe" : "")}>";
    }

    private string MapType(CppCompilation compilation, CppType type, bool isUnsafe = false, bool forInterop = false, bool isReturnValue = false)
    {
        var typeName = type.GetDisplayName().Replace("Elem", string.Empty);

        Console.WriteLine($"MapType: {typeName}");

        return typeName switch
        {
            "Application" => "ElementalApplication",
            "char const *" => !isUnsafe ? "ReadOnlySpan<byte>" : "byte*",
            "unsigned long long" => "UInt64",
            "unsigned int" => "uint",
            "DataSpan" => !forInterop ? "ReadOnlySpan<byte>" : "SpanUnsafe<byte>", 
            string value when value.Contains("Span") && isUnsafe => $"{value.Substring(0, value.IndexOf("Span"))}*",
            string value when value.Contains("Span") && forInterop => GetSpanUnsafe(compilation, value.Substring(0, value.IndexOf("Span"))),
            string value when value.Contains("Span") && isReturnValue => $"ReadOnlySpan<{value.Substring(0, value.IndexOf("Span"))}>",
            string value when value.Contains("Span") => $"ReadOnlySpan<{value.Substring(0, value.IndexOf("Span"))}>",
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
