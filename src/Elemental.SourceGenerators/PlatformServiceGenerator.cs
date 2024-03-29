﻿using System.Collections.Immutable;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace Elemental.SourceGenerators;

record PlatformServiceToGenerate
{
    public string? Namespace { get; set; }
    public string InterfaceName { get; set; } = "";
    public string InitMethod { get; set; } = "";
    public string InitMethodParameters { get; set; } = "";
    public string InitMethodParametersValues { get; set; } = "";
    public string DisposeMethod { get; set; } = "";

    public IList<IMethodSymbol> MethodList { get; } = new List<IMethodSymbol>();

    public string ImplementationClassName
    {
        get
        {
            return InterfaceName.Substring(1);
        }
    }

    public string InteropClassName
    {
        get
        {
            return $"{ImplementationClassName}Interop";
        }
    }
}

// TODO: Write utils method to auto indent generated code
// TODO: Implement a way to have custom methods in the partial class
// and add an attribute [PlatformServiceCustom]
[Generator]
public class PlatformServiceGenerator : IIncrementalGenerator
{
    public void Initialize(IncrementalGeneratorInitializationContext context)
    {
        GenerateAttributes(context);

        var interfaceDeclarations = context.SyntaxProvider
            .CreateSyntaxProvider(
                predicate: static (syntaxNode, _) => FilterInterfaceNodes(syntaxNode),
                transform: static (context, _) => FilterPlatformServices(context))
            .Where(static item => item is not null);

        var compilationAndIntefaces = context.CompilationProvider.Combine(interfaceDeclarations.Collect());

        context.RegisterImplementationSourceOutput(compilationAndIntefaces,
            static (context, source) => Generate(source.Left, source.Right, context));
    }

    private static void GenerateAttributes(IncrementalGeneratorInitializationContext context)
    {
        var attributeCode = """
                            namespace Elemental;

                            /// <summary>
                            /// Attribute used by the source generator to generate native service interop code.
                            /// </summary>
                            [AttributeUsage(AttributeTargets.Interface)]
                            public class PlatformServiceAttribute : Attribute
                            {
                                /// <summary>
                                /// Default constructor.
                                /// </summary>
                                public PlatformServiceAttribute()
                                {
                                    InitMethod = null;
                                    DisposeMethod = null;
                                }

                                /// <summary>
                                /// Gets or sets the name of the init method to use when the platform service is first created.
                                /// </summary>
                                /// <value>Name of the init method.</value>
                                public string InitMethod { get; set; }

                                /// <summary>
                                /// Gets or sets the name of the dispose method to use when the platform service is disposed.
                                /// </summary>
                                /// <value>Name of the dispose method.</value>
                                public string DisposeMethod { get; set; }
                            }
                            """;

        context.RegisterPostInitializationOutput(context => context.AddSource(
            "PlatformServiceAttribute.g.cs",
            attributeCode));

        var overrideAttributeCode = """
                                    namespace Elemental;

                                    /// <summary>
                                    /// Attribute used by the source generator to generate a native service method override interop code.
                                    /// </summary>
                                    [AttributeUsage(AttributeTargets.Method)]
                                    public class PlatformMethodOverrideAttribute : Attribute
                                    {
                                    }
                                    """;

        context.RegisterPostInitializationOutput(context => context.AddSource(
            "PlatformMethodOverrideAttribute.g.cs",
            overrideAttributeCode));
            
            
        var ignoreAttributeCode =   """
                                    namespace Elemental;

                                    /// <summary>
                                    /// Attribute used by the source generator that tell the generator to ignore the method.
                                    /// </summary>
                                    [AttributeUsage(AttributeTargets.Method)]
                                    public class PlatformMethodIgnoreAttribute : Attribute
                                    {
                                    }
                                    """;

        context.RegisterPostInitializationOutput(context => context.AddSource(
            "PlatformMethodIgnoreAttribute.g.cs",
            ignoreAttributeCode));
    }

    private static bool FilterInterfaceNodes(SyntaxNode syntaxNode)
    {
        return syntaxNode is InterfaceDeclarationSyntax interfaceNode && interfaceNode.AttributeLists.Count > 0;
    }

    private static InterfaceDeclarationSyntax? FilterPlatformServices(GeneratorSyntaxContext context)
    {
        var interfaceDeclarationSyntax = (InterfaceDeclarationSyntax)context.Node;

        foreach (var attributeListSyntax in interfaceDeclarationSyntax.AttributeLists)
        {
            foreach (var attributeSyntax in attributeListSyntax.Attributes)
            {
                if (context.SemanticModel.GetSymbolInfo(attributeSyntax).Symbol is not IMethodSymbol attributeSymbol)
                {
                    continue;
                }

                if (attributeSymbol.ContainingType.ToDisplayString() == "Elemental.PlatformServiceAttribute")
                {
                    return interfaceDeclarationSyntax;
                }
            }
        }

        return null;
    }

    private static void Generate(Compilation compilation, ImmutableArray<InterfaceDeclarationSyntax?> interfaces, SourceProductionContext context)
    {
        if (interfaces.IsDefaultOrEmpty)
        {
            return;
        }

        var distinctInterfaces = interfaces.Where(item => item is not null)
                                           .Select(item => item!)
                                           .Distinct();

        var interfacesToGenerate = GetTypesToGenerate(compilation, distinctInterfaces, context.CancellationToken);

        if (interfacesToGenerate.Count > 0)
        {
            foreach (var platformService in interfacesToGenerate)
            {
                var sourceCode = new StringBuilder();
                GenerateImplementationClass(sourceCode, platformService);
                context.AddSource($"{platformService.ImplementationClassName}.g.cs", SourceText.From(sourceCode.ToString(), Encoding.UTF8));

                // TODO: Generators cannot be chained for now.
                // Check issue for follow up: https://github.com/dotnet/roslyn/issues/57239
                //sourceCode.Clear();
                //GenerateInteropClass(sourceCode, platformService);
                //context.AddSource($"{platformService.InteropClassName}.g.cs", SourceText.From(sourceCode.ToString(), Encoding.UTF8));
            }
        }
    }

    private static void GenerateImplementationClass(StringBuilder sourceCode, PlatformServiceToGenerate platformService)
    {
        if (platformService.Namespace is not null)
        {
            sourceCode.AppendLine($"namespace {platformService.Namespace};");
            sourceCode.AppendLine();
        }

        sourceCode.AppendLine($"/// <inheritdoc cref=\"{platformService.InterfaceName}\" />");
        sourceCode.AppendLine($"public partial class {platformService.ImplementationClassName} : {platformService.InterfaceName}{((platformService.DisposeMethod != "null") ? ", IDisposable" : "")}");
        sourceCode.AppendLine("{");

        if (platformService.InitMethod != "null")
        {
            sourceCode.AppendLine("/// <summary>Default constructor.</summary>");
            sourceCode.AppendLine($"public {platformService.ImplementationClassName}({platformService.InitMethodParameters})");
            sourceCode.AppendLine("{");
            sourceCode.AppendLine($"PlatformServiceInterop.{platformService.InitMethod.Replace("\"", "")}({platformService.InitMethodParametersValues});");
            sourceCode.AppendLine("}");
        }

        if (platformService.DisposeMethod != "null")
        {
            sourceCode.AppendLine("/// <summary>Frees any unmanaged resources owned by the service.</summary>");
            sourceCode.AppendLine($"public void Dispose()");
            sourceCode.AppendLine("{");
            sourceCode.AppendLine($"PlatformServiceInterop.{platformService.DisposeMethod.Replace("\"", "")}();");
            sourceCode.AppendLine("}");
        }

        foreach (var method in platformService.MethodList)
        {
            var methodName = method.Name;
            var isOverride = false;

            if (method.GetAttributes().Any(item => item.AttributeClass?.Name == "PlatformMethodOverrideAttribute"))
            {
                methodName += "Implementation";
                isOverride = true;
            }

            sourceCode.AppendLine($"/// <inheritdoc cref=\"{platformService.InterfaceName}\" />");
            sourceCode.AppendLine($"{(!isOverride ? "public" : "private")} unsafe {((INamedTypeSymbol)method.ReturnType).ToString()} {methodName}({string.Join(",", method.Parameters.Select(item => GenerateParameterValue(item, isMethodDefinition: true)))})");
            sourceCode.AppendLine("{");

            var isReturnTypeNativePointer = method.ReturnType.GetAttributes().Any(item => item.AttributeClass?.Name == "PlatformNativePointerAttribute");
            var isReturnTypeSpan = method.ReturnType.Name == "Span" || method.ReturnType.Name == "ReadOnlySpan";

            if (isReturnTypeNativePointer)
            {
                sourceCode.Append("var result = ");
            }

            else if (isReturnTypeSpan)
            {
                var structName = ((INamedTypeSymbol)method.ReturnType).TypeArguments[0].Name;

                // TODO: This is really hacky, this is temporary
                sourceCode.AppendLine("int outputCount = 0;");
                sourceCode.AppendLine($"var convertedResult = Array.Empty<{structName}>();");
                sourceCode.AppendLine($"Span<{structName}Marshaller.{structName}Unmanaged> result = stackalloc {structName}Marshaller.{structName}Unmanaged[50];");
                sourceCode.AppendLine($"fixed({structName}Marshaller.{structName}Unmanaged* nativeResult = result)");
                sourceCode.AppendLine("{");

                sourceCode.AppendLine("try");
                sourceCode.AppendLine("{");
            }

            else if (method.ReturnType.Name.ToLower() != "void" && !isReturnTypeSpan)
            {
                sourceCode.Append("return ");
            }

            var parameterList = method.Parameters.Select(item => GenerateParameterValue(item, isMethodDefinition: false)).ToList();
            parameterList.AddRange(GetSpanParameters(isReturnTypeSpan));
            sourceCode.AppendLine($"PlatformServiceInterop.Native_{method.Name}({string.Join(",", parameterList)});");

            if (isReturnTypeNativePointer)
            {
                sourceCode.AppendLine("if (result.NativePointer == nint.Zero)");
                sourceCode.AppendLine("{");
                sourceCode.AppendLine($"throw new InvalidOperationException(\"There was an error when executing '{methodName}'.\");");
                sourceCode.AppendLine("}");
                sourceCode.AppendLine("return result;");
            }

            else if (isReturnTypeSpan)
            {
                var structName = ((INamedTypeSymbol)method.ReturnType).TypeArguments[0].Name;

                sourceCode.AppendLine($"convertedResult = new {structName}[outputCount];");
                sourceCode.AppendLine("for (var i = 0; i < outputCount; i++)");
                sourceCode.AppendLine("{");
                sourceCode.AppendLine($"convertedResult[i] = {structName}Marshaller.ConvertToManaged(result[i]);");
                sourceCode.AppendLine("}");

                sourceCode.AppendLine("}");
                sourceCode.AppendLine("finally");
                sourceCode.AppendLine("{");
                sourceCode.AppendLine("for (var i = 0; i < outputCount; i++)");
                sourceCode.AppendLine("{");
                sourceCode.AppendLine($"{structName}Marshaller.Free(result[i]);");
                sourceCode.AppendLine("}");

                sourceCode.AppendLine("}");

                sourceCode.AppendLine("return convertedResult;");
                sourceCode.AppendLine("}");

                sourceCode.AppendLine("throw new InvalidOperationException();");
                // TODO: Cleanup

            }

            sourceCode.AppendLine("}");
        }

        sourceCode.AppendLine("}");
    }

    private static string GenerateParameterValue(IParameterSymbol item, bool isMethodDefinition)
    {
        var result = GenerateReferenceType(item);
        var typeName = ((INamedTypeSymbol)item.Type).ToString();

        if (item.RefKind != RefKind.None && !(!isMethodDefinition && item.HasExplicitDefaultValue && item.ExplicitDefaultValue == null))
        {
            result += item.RefKind.ToString().ToLower() + " ";
        }

        if (isMethodDefinition)
        {
            result += typeName + " ";
        }

        result += item.Name;

        if (isMethodDefinition && item.HasExplicitDefaultValue && item.ExplicitDefaultValue != null)
        {
            result += " = " + item.ExplicitDefaultValue;
        }

        else if (isMethodDefinition && item.HasExplicitDefaultValue && item.ExplicitDefaultValue == null)
        {
            result += $" = default({typeName})";
        }

        else if (!isMethodDefinition && item.HasExplicitDefaultValue && item.ExplicitDefaultValue == null)
        {
            result += $" == default({typeName}) ? new {typeName}() : {item.Name}";
        }

        if (!isMethodDefinition && ((INamedTypeSymbol)item.Type).Name == "ReadOnlySpan")
        {
            result += $", {item.Name}.Length";
        }

        return result;
    }

    private static IEnumerable<string> GetSpanParameters(bool generateParameters)
    {
        if (generateParameters)
        {
            yield return "nativeResult";
            yield return "out outputCount";
        }
    }

    private static string GenerateReferenceType(IParameterSymbol item)
    {
        return item.RefKind switch
        {
            RefKind.Out => "out ",
            RefKind.Ref => "ref ",
            _ => string.Empty
        };
    }

    private static void GenerateInteropClass(StringBuilder sourceCode, PlatformServiceToGenerate platformService)
    {
        sourceCode.AppendLine("using System.Runtime.InteropServices;");
        sourceCode.AppendLine();

        if (platformService.Namespace is not null)
        {
            sourceCode.AppendLine($"namespace {platformService.Namespace};");
            sourceCode.AppendLine();
        }

        sourceCode.AppendLine($"internal static partial class {platformService.InteropClassName}");
        sourceCode.AppendLine("{");

        foreach (var method in platformService.MethodList)
        {
            sourceCode.AppendLine("[LibraryImport(\"Elemental.Native\", StringMarshalling = StringMarshalling.Utf8)]");
            sourceCode.AppendLine($"internal static partial {((INamedTypeSymbol)method.ReturnType).ToString()} Native_{method.Name}({string.Join(",", method.Parameters.Select(item => ((INamedTypeSymbol)item.Type).ToString() + " " + item.Name))});");
            sourceCode.AppendLine();
        }

        sourceCode.AppendLine("}");
    }

    static List<PlatformServiceToGenerate> GetTypesToGenerate(Compilation compilation, IEnumerable<InterfaceDeclarationSyntax> interfaces, CancellationToken cancellationToken)
    {
        var result = new List<PlatformServiceToGenerate>();

        var platformServiceAttribute = compilation.GetTypeByMetadataName("Elemental.PlatformServiceAttribute");

        if (platformServiceAttribute == null)
        {
            return result;
        }

        var platformInteropClass = compilation.GetTypeByMetadataName("Elemental.PlatformServiceInterop");

        if (platformInteropClass == null)
        {
            return result;
        }

        foreach (var interfaceDeclarationSyntax in interfaces)
        {
            cancellationToken.ThrowIfCancellationRequested();

            var semanticModel = compilation.GetSemanticModel(interfaceDeclarationSyntax.SyntaxTree);

            if (semanticModel.GetDeclaredSymbol(interfaceDeclarationSyntax, cancellationToken) is not INamedTypeSymbol interfaceSymbol)
            {
                continue;
            }

            var interfaceName = interfaceSymbol.Name;
            var namespaceName = interfaceSymbol.ContainingNamespace;
            var attribute = interfaceSymbol.GetAttributes().First(item => item.AttributeClass!.Name == "PlatformServiceAttribute");

            if (interfaceName is null)
            {
                continue;
            }

            var initMethodName = attribute.NamedArguments.FirstOrDefault(item => item.Key == "InitMethod").Value.ToCSharpString();
            var initMethod = (IMethodSymbol?)platformInteropClass.GetMembers().FirstOrDefault(member => member.Name == initMethodName.Replace("\"", ""));
            var methodParameters = string.Empty;
            var methodParametersValues = string.Empty;

            if (initMethod is not null)
            {
                methodParameters = $"{string.Join(",", initMethod.Parameters.Select(item => GenerateParameterValue(item, isMethodDefinition: true)))}";
                methodParametersValues = $"{string.Join(",", initMethod.Parameters.Select(item => GenerateParameterValue(item, isMethodDefinition: false)))}";
            }

            var platformService = new PlatformServiceToGenerate
            {
                InterfaceName = interfaceName,
                Namespace = namespaceName?.ToString(),
                InitMethod = initMethodName,
                InitMethodParameters = methodParameters,
                InitMethodParametersValues = methodParametersValues,
                DisposeMethod = attribute.NamedArguments.FirstOrDefault(item => item.Key == "DisposeMethod").Value.ToCSharpString()
            };

            var members = interfaceSymbol.GetMembers();

            foreach (var member in members)
            {
                if (member is IMethodSymbol method)
                {
                    if (!method.GetAttributes().Any(item => item.AttributeClass!.Name == "PlatformMethodIgnoreAttribute"))
                    {
                        platformService.MethodList.Add(method);
                    }
                }
            }

            result.Add(platformService);
        }

        return result;
    }
}