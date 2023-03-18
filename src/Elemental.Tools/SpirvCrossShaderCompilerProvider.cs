using System.Diagnostics;

namespace Elemental.Tools;

internal class SpirvCrossShaderCompilerProvider : IShaderCompilerProvider
{
    private readonly string _spirvCrossPath;

    public ShaderLanguage ShaderLanguage => ShaderLanguage.Spirv;
    public ReadOnlySpan<ShaderLanguage> TargetShaderLanguages => new[] { ShaderLanguage.Msl };
    
    public SpirvCrossShaderCompilerProvider()
    {
        var processPath = Path.GetDirectoryName(Environment.ProcessPath)!;
        _spirvCrossPath = string.Empty;

        if (OperatingSystem.IsWindows())
        {
            _spirvCrossPath = Path.Combine(processPath, "ShaderCompilers", "spirv-cross.exe");
        }
        else if (OperatingSystem.IsMacOS())
        {
            _spirvCrossPath = Path.Combine(processPath, "ShaderCompilers", "bin", "spirv-cross");
        }
    }

    public bool IsCompilerInstalled()
    {
        return File.Exists(_spirvCrossPath);
    }
    
    public ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        var inputFilePath = $"{Path.GetTempFileName()}.spirv";
        var outputFilePath = Path.GetTempFileName();

        File.WriteAllBytes(inputFilePath, shaderCode.ToArray());
        
        var arguments = new List<string>()
        {
            "--msl"
        };
        
        using var process = Process.Start(new ProcessStartInfo 
        { 
            FileName = _spirvCrossPath, 
            Arguments = $"{inputFilePath} --output {outputFilePath} {string.Join(' ', arguments.ToArray())}", 
            RedirectStandardError = true, 
            RedirectStandardOutput = true
        });

        if (process == null)
        {
            return ShaderCompilerResult.CreateErrorResult("Cannot start SPIRV-Cross shader compiler process.");
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