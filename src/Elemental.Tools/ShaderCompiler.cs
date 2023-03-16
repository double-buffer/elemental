using System.Buffers;

namespace Elemental.Tools;

/// <inheritdoc cref="IShaderCompiler" />
public class ShaderCompiler : IShaderCompiler
{
    private readonly List<IShaderCompilerProvider> _shaderCompilerProviders;
    
    // TODO: Convert that to frozen dicitonary in .NET 8 :)
    private readonly Dictionary<ToolsGraphicsApi, ShaderLanguage> _platformTargetLanguages;

    /// <summary>
    /// Default constructor.
    /// </summary>
    public ShaderCompiler()
    {
        _shaderCompilerProviders = new List<IShaderCompilerProvider>();
        _platformTargetLanguages = new()
        {
            { ToolsGraphicsApi.Direct3D12, ShaderLanguage.Dxil },
            { ToolsGraphicsApi.Vulkan, ShaderLanguage.Spirv },
            { ToolsGraphicsApi.Metal, ShaderLanguage.MetalIR }
        };

        RegisterShaderCompilerProvider(new DirectXShaderCompilerProvider());
        RegisterShaderCompilerProvider(new SpirvCrossShaderCompilerProvider());
        
        if (OperatingSystem.IsMacOS())
        {
            RegisterShaderCompilerProvider(new MetalShaderCompilerProvider());
        }
    }

    /// <inheritdoc cref="IShaderCompiler" />
    public bool CanCompileShader(ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        if (!_platformTargetLanguages.ContainsKey(graphicsApi))
        {
            return false;
        }

        var graphicsApiTargetShaderLanguage = _platformTargetLanguages[graphicsApi];

        var shaderCompilerProviders = ArrayPool<IShaderCompilerProvider>.Shared.Rent(10);
        var result = FindShaderCompilersChain(shaderCompilerProviders, 0, shaderLanguage, graphicsApiTargetShaderLanguage);
        ArrayPool<IShaderCompilerProvider>.Shared.Return(shaderCompilerProviders);

        return result;
    }

    /// <inheritdoc cref="IShaderCompiler" />
    public ShaderCompilerResult CompileShader(ReadOnlySpan<char> shaderCode)
    {
        throw new NotImplementedException();
    }

    private void RegisterShaderCompilerProvider(IShaderCompilerProvider shaderCompilerProvider)
    {
        if (shaderCompilerProvider.IsCompilerInstalled())
        {
            _shaderCompilerProviders.Add(shaderCompilerProvider);
        }
    }

    private bool FindShaderCompilersChain(Span<IShaderCompilerProvider> shaderCompilers, int currentLevel, ShaderLanguage sourceLanguage, ShaderLanguage targetLanguage)
    {
        var compiler = FindShaderCompiler(targetLanguage);

        if (compiler == null)
        {
            return false;
        }

        shaderCompilers[currentLevel] = compiler;

        if (compiler.ShaderLanguage == sourceLanguage)
        {
            return true;
        }
        else
        {
            return FindShaderCompilersChain(shaderCompilers, currentLevel + 1, sourceLanguage, compiler.ShaderLanguage);
        }
    }

    private IShaderCompilerProvider? FindShaderCompiler(ShaderLanguage targetShaderLanguage)
    {
        foreach (var shaderCompilerProvider in _shaderCompilerProviders)
        {
            foreach (var providerTargetShaderLanguage in shaderCompilerProvider.TargetShaderLanguages)
            {
                if (providerTargetShaderLanguage == targetShaderLanguage)
                {
                    return shaderCompilerProvider;
                }
            }
        }

        return null;
    }
}