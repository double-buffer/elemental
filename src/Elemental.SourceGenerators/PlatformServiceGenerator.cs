using System.Collections.Immutable;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace Elemental.SourceGenerators;

record PlatformServiceToGenerate
{
    public required string? Namespace { get; init; }
    public required string InterfaceName { get; init; }

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
        sourceCode.AppendLine($"public partial class {platformService.ImplementationClassName} : {platformService.InterfaceName}");
        sourceCode.AppendLine("{");

        foreach (var method in platformService.MethodList)
        {
            var methodName = method.Name;

            if (method.GetAttributes().Any(item => item.AttributeClass?.Name == "PlatformMethodOverrideAttribute"))
            {
                methodName += "Implementation";
            }

            sourceCode.AppendLine($"/// <inheritdoc cref=\"{platformService.InterfaceName}\" />");
            sourceCode.AppendLine($"public {((INamedTypeSymbol)method.ReturnType).ToString()} {methodName}({string.Join(',', method.Parameters.Select(item => GenerateReferenceType(item) + ((INamedTypeSymbol)item.Type).ToString() + " " + item.Name))})");
            sourceCode.AppendLine("{");
            
            if (method.ReturnType.Name.ToLower() != "void")
            {
                sourceCode.Append("return ");
            }

            sourceCode.AppendLine($"{platformService.InteropClassName}.Native_{method.Name}({string.Join(',', method.Parameters.Select(item => GenerateReferenceType(item) + item.Name))});");
            
            sourceCode.AppendLine("}");
        }

        sourceCode.AppendLine("}");
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
            sourceCode.AppendLine($"internal static partial {((INamedTypeSymbol)method.ReturnType).ToString()} Native_{method.Name}({string.Join(',', method.Parameters.Select(item => ((INamedTypeSymbol) item.Type).ToString() + " " + item.Name))});");
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

            if (interfaceName is null)
            {
                continue;
            }
            
            var platformService = new PlatformServiceToGenerate
            {
                InterfaceName = interfaceName,
                Namespace = namespaceName?.ToString()
            };

            var members = interfaceSymbol.GetMembers();

            foreach (var member in members)
            {
                if (member is IMethodSymbol method)
                {
                    platformService.MethodList.Add(method);
                }
            }

            result.Add(platformService);
        }

        return result;
    }
}