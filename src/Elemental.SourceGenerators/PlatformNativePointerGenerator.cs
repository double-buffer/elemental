﻿using System.Collections.Immutable;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace Elemental.SourceGenerators;

record PlatformNativePointerToGenerate
{
    public string? Namespace { get; set; }
    public string StructName { get; set; } = "";
    public string DeleteMethod { get; set; } = "";
}

[Generator]
public class PlatformNativePointerGenerator : IIncrementalGenerator
{
    public void Initialize(IncrementalGeneratorInitializationContext context)
    {
        GenerateAttribute(context);

        var structDeclarations = context.SyntaxProvider
            .CreateSyntaxProvider(
                predicate: static (syntaxNode, _) => FilterStructNodes(syntaxNode),
                transform: static (context, _) => FilterPlatformNativePointer(context))
            .Where(static item => item is not null);

        var compilationAndStructs = context.CompilationProvider.Combine(structDeclarations.Collect());

        context.RegisterSourceOutput(compilationAndStructs,
            static (context, source) => Generate(source.Left, source.Right, context));
    }

    private static void GenerateAttribute(IncrementalGeneratorInitializationContext context)
    {
        var attributeCode = """
                            namespace Elemental;

                            /// <summary>
                            /// Attribute used by the source generator to generate native pointer interop code.
                            /// </summary>
                            [AttributeUsage(AttributeTargets.Struct)]
                            public class PlatformNativePointerAttribute : Attribute
                            {
                                /// <summary>
                                /// Default constructor.
                                /// </summary>
                                public PlatformNativePointerAttribute()
                                {
                                    DeleteMethod = null;
                                }

                                /// <summary>
                                /// Gets or sets the name of the delete method to use when the Native pointer is disposed.
                                /// </summary>
                                /// <value>Name of the delete method.</value>
                                public string DeleteMethod { get; set; }
                            }
                            """;

        context.RegisterPostInitializationOutput(context => context.AddSource(
            "PlatformNativePointerAttribute.g.cs",
            attributeCode));
    }

    private static bool FilterStructNodes(SyntaxNode syntaxNode)
    {
        return syntaxNode is RecordDeclarationSyntax structNode && structNode.AttributeLists.Count > 0;
    }

    private static RecordDeclarationSyntax? FilterPlatformNativePointer(GeneratorSyntaxContext context)
    {
        var structDeclarationSyntax = (RecordDeclarationSyntax)context.Node;

        // TODO: Check partial modifier

        foreach (var attributeListSyntax in structDeclarationSyntax.AttributeLists)
        {
            foreach (var attributeSyntax in attributeListSyntax.Attributes)
            {
                if (context.SemanticModel.GetSymbolInfo(attributeSyntax).Symbol is not IMethodSymbol attributeSymbol)
                {
                    continue;
                }

                if (attributeSymbol.ContainingType.ToDisplayString() == "Elemental.PlatformNativePointerAttribute")
                {
                    return structDeclarationSyntax;
                }
            }
        }

        return null;
    }

    private static void Generate(Compilation compilation, ImmutableArray<RecordDeclarationSyntax?> structs, SourceProductionContext context)
    {
        if (structs.IsDefaultOrEmpty)
        {
            return;
        }

        var distinctStructs = structs.Where(item => item is not null)
                                     .Select(item => item!)
                                     .Distinct();

        var structsToGenerate = GetTypesToGenerate(compilation, distinctStructs, context.CancellationToken);

        if (structsToGenerate.Count > 0)
        {
            foreach (var structToGenerate in structsToGenerate)
            {
                var sourceCode = new StringBuilder();
                GenerateImplementationClass(sourceCode, structToGenerate);
                context.AddSource($"{structToGenerate.Namespace}.{structToGenerate.StructName}.g.cs", SourceText.From(sourceCode.ToString(), Encoding.UTF8));
            }
        }
    }

    private static void GenerateImplementationClass(StringBuilder sourceCode, PlatformNativePointerToGenerate platformService)
    {
        if (platformService.Namespace is not null)
        {
            sourceCode.AppendLine($"namespace {platformService.Namespace};");
            sourceCode.AppendLine();
        }

        var implementationCode = """
                                 public partial record struct ##NAME####INHERIT##
                                 {
                                    /// <summary>
                                    /// Gets the native pointer used by this handle.
                                    /// </summary>
                                    /// <value>Native pointer.</value>
                                    public nint NativePointer { get; init; }

                                    ##DISPOSE##
                                 }
                                 """;

        sourceCode.AppendLine(implementationCode);

        var disposeCode = """
                            /// <summary>
                            /// Disposes the resource owned by the platform pointer.
                            /// </summary>
                            public void Dispose()
                            {
                                if (NativePointer != nint.Zero)
                                {
                                    PlatformServiceInterop.##DELETEMETHOD##(this);
                                }
                            }
                          """;

        sourceCode.Replace("##INHERIT##", platformService.DeleteMethod != "null" ? " : IDisposable" : string.Empty);
        sourceCode.Replace("##DISPOSE##", platformService.DeleteMethod != "null" ? disposeCode : string.Empty);
        sourceCode.Replace("##NAME##", platformService.StructName);
        sourceCode.Replace("##DELETEMETHOD##", platformService.DeleteMethod.Replace("\"", string.Empty));
    }

    static List<PlatformNativePointerToGenerate> GetTypesToGenerate(Compilation compilation, IEnumerable<RecordDeclarationSyntax> structs, CancellationToken cancellationToken)
    {
        var result = new List<PlatformNativePointerToGenerate>();
        
        var platformNativePointerAttribute = compilation.GetTypeByMetadataName("Elemental.PlatformNativePointerAttribute");

        if (platformNativePointerAttribute == null)
        {
            return result;
        }

        foreach (var structDeclarationSyntax in structs)
        {
            cancellationToken.ThrowIfCancellationRequested();

            var semanticModel = compilation.GetSemanticModel(structDeclarationSyntax.SyntaxTree);

            if (semanticModel.GetDeclaredSymbol(structDeclarationSyntax, cancellationToken) is not INamedTypeSymbol structSymbol)
            {
                continue;
            }
            
            var structName = structSymbol.Name;
            var namespaceName = structSymbol.ContainingNamespace;
            var attribute = structSymbol.GetAttributes().First(item => item.AttributeClass!.Name == "PlatformNativePointerAttribute");

            if (structName is null)
            {
                continue;
            }
            
            var platformNativePointer = new PlatformNativePointerToGenerate
            {
                StructName = structName,
                Namespace = namespaceName?.ToString(),
                DeleteMethod = attribute.NamedArguments.FirstOrDefault(item => item.Key == "DeleteMethod").Value.ToCSharpString()
            };

            result.Add(platformNativePointer);
        }

        return result;
    }
}