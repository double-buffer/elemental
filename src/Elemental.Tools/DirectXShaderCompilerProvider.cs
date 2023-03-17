using System.Diagnostics;
using System.Runtime.CompilerServices;

namespace Elemental.Tools;

internal class DirectXShaderCompilerProvider : IShaderCompilerProvider
{
    private readonly string _dxcPath;

    public ShaderLanguage ShaderLanguage => ShaderLanguage.Hlsl;
    public ReadOnlySpan<ShaderLanguage> TargetShaderLanguages => new[] { ShaderLanguage.Dxil, ShaderLanguage.Spirv };

    public DirectXShaderCompilerProvider()
    {
        var processPath = Path.GetDirectoryName(Environment.ProcessPath)!;
        _dxcPath = string.Empty;

        if (OperatingSystem.IsWindows())
        {
            _dxcPath = Path.Combine(processPath, "ShaderCompilers", "dxc.exe");
        }
        else if (OperatingSystem.IsMacOS())
        {
            _dxcPath = Path.Combine(processPath, "ShaderCompilers", "dxc");
        }
    }

    public bool IsCompilerInstalled()
    {
        return File.Exists(_dxcPath);
    }

    public unsafe ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        var inputFilePath = $"{Path.GetTempFileName()}.hlsl";
        var outputFilePath = Path.GetTempFileName();

        File.WriteAllBytes(inputFilePath, shaderCode.ToArray());
        
        var arguments = new List<string>()
        {
            "-HV 2021"
        };

        if (graphicsApi != ToolsGraphicsApi.Direct3D12)
        {
            arguments.Add("-spirv");
            arguments.Add("-fspv-target-env=vulkan1.3");
        }
        else
        {
            arguments.Add("-rootsig-define RootSignatureDef");
        }

        var shaderTarget = "ms_6_7";

        if (shaderStage == ToolsShaderStage.AmplificationShader)
        {
            shaderTarget = "as_6_7";
        }
        else if (shaderStage == ToolsShaderStage.PixelShader)
        {
            shaderTarget = "ps_6_7";
        }

        using var process = Process.Start(new ProcessStartInfo 
        { 
            FileName = _dxcPath, 
            Arguments = $"-T {shaderTarget} -E {entryPoint} -Fo {outputFilePath} {string.Join(' ', arguments.ToArray())} {inputFilePath}", 
            RedirectStandardError = true, 
            RedirectStandardOutput = true
        });

        if (process == null)
        {
            return ShaderCompilerResult.CreateErrorResult("Cannot start Metal shader compiler process.");
        }

        process.WaitForExit();
        string? line;
        var logList = new List<ShaderCompilerLogEntry>();
        var currentLogType = ShaderCompilerLogEntryType.Error;
        var hasErrors = false;

        while ((line = process.StandardError.ReadLine()) != null)
        {
            if (line.Contains("warning:"))
            {
                currentLogType = ShaderCompilerLogEntryType.Warning;
            }
            else if (line.Contains("error:"))
            {
                currentLogType = ShaderCompilerLogEntryType.Error;
                hasErrors = true;
            }

            logList.Add(new() { Type = currentLogType, Message = line });
        }
        
        while ((line = process.StandardOutput.ReadLine()) != null)
        {
            logList.Add(new() { Type = ShaderCompilerLogEntryType.Message, Message = line });
        }
        
        var shaderBytecode = File.ReadAllBytes(outputFilePath);

        File.Delete(inputFilePath);
        File.Delete(outputFilePath);

        return new ShaderCompilerResult
        {
            IsSuccess = !hasErrors,
            LogEntries = logList.ToArray(),
            ShaderData = shaderBytecode
        };
    }
}