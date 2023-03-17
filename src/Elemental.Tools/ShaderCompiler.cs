using System.Buffers;
using System.Text;

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

        return result > 0;
    }

    /// <inheritdoc cref="IShaderCompiler" />
    public ShaderCompilerResult CompileShader(ReadOnlySpan<char> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        // TODO: Review the ToArray() usages

        if (!_platformTargetLanguages.ContainsKey(graphicsApi))
        {
            throw new InvalidOperationException("Graphics API is not supported!");
        }

        var graphicsApiTargetShaderLanguage = _platformTargetLanguages[graphicsApi];

        var shaderCompilerProviders = ArrayPool<IShaderCompilerProvider>.Shared.Rent(10);
        var result = FindShaderCompilersChain(shaderCompilerProviders, 0, shaderLanguage, graphicsApiTargetShaderLanguage);

        if (result > 0)
        {
            var currentShaderInput = Encoding.UTF8.GetBytes(shaderCode.ToString());

            var isSuccess = false;
            var logList = new List<ShaderCompilerLogEntry>();
            ReadOnlyMemory<byte>? currentShaderData = null;

            var providers = shaderCompilerProviders.AsSpan(0, result);
            providers.Reverse();

            foreach (var shaderCompilerProvider in providers)
            {
                var compilationResult = shaderCompilerProvider.CompileShader(currentShaderInput, shaderStage, entryPoint, shaderLanguage, graphicsApi);
                logList.AddRange(compilationResult.LogEntries.ToArray());

                // TODO: Check errors
                currentShaderData = compilationResult.ShaderData;
                isSuccess = compilationResult.IsSuccess;
            }

            if (currentShaderData != null)
            { 
                return new ShaderCompilerResult
                {
                    IsSuccess = isSuccess,
                    LogEntries = logList.ToArray(),
                    ShaderData = currentShaderData.Value
                };
            }
        }
        
        ArrayPool<IShaderCompilerProvider>.Shared.Return(shaderCompilerProviders);

        return new ShaderCompilerResult
        {
            IsSuccess = false,
            LogEntries = new ShaderCompilerLogEntry[]
            {
                new() { Type = ShaderCompilerLogEntryType.Error, Message = "Cannot find compatible shader compilers." }
            }
        };
    }

    private void RegisterShaderCompilerProvider(IShaderCompilerProvider shaderCompilerProvider)
    {
        if (shaderCompilerProvider.IsCompilerInstalled())
        {
            _shaderCompilerProviders.Add(shaderCompilerProvider);
        }
    }

    private int FindShaderCompilersChain(Span<IShaderCompilerProvider> shaderCompilers, int currentLevel, ShaderLanguage sourceLanguage, ShaderLanguage targetLanguage)
    {
        var compiler = FindShaderCompiler(targetLanguage);

        if (compiler == null)
        {
            return 0;
        }

        shaderCompilers[currentLevel] = compiler;

        if (compiler.ShaderLanguage == sourceLanguage)
        {
            return 1;
        }
        else
        {
            var result = FindShaderCompilersChain(shaderCompilers, currentLevel + 1, sourceLanguage, compiler.ShaderLanguage);

            if (result > 0)
            {
                return result + 1;
            }

            return 0;
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