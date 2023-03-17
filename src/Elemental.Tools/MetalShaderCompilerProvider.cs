using System.Diagnostics;

namespace Elemental.Tools;

// TODO: It would be gread in the future to be able to have a standalone install on MacOS :/
// With a library instead of an executable

internal class MetalShaderCompilerProvider : IShaderCompilerProvider
{
    public ShaderLanguage ShaderLanguage => ShaderLanguage.Msl;
    public ReadOnlySpan<ShaderLanguage> TargetShaderLanguages => new[] { ShaderLanguage.MetalIR };

    public bool IsCompilerInstalled()
    {
        using var process = Process.Start(new ProcessStartInfo 
        { 
            FileName = "xcrun", 
            Arguments = "-f metal", 
            RedirectStandardError = true, 
            RedirectStandardOutput = true
        });

        if (process == null)
        {
            return false;
        }

        process.WaitForExit();
        return string.IsNullOrEmpty(process.StandardError.ReadToEnd());
    }
    
    public ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        var inputFilePath = $"{Path.GetTempFileName()}.metal";
        var airFilePath = Path.GetTempFileName();
        var outputFilePath = Path.GetTempFileName();

        File.WriteAllBytes(inputFilePath, shaderCode.ToArray());

        using var process = Process.Start(new ProcessStartInfo 
        { 
            FileName = "xcrun", 
            Arguments = $"metal -c {inputFilePath} -o {airFilePath}", 
            RedirectStandardError = true, 
            RedirectStandardOutput = true
        });

        if (process == null)
        {
            return new ShaderCompilerResult
            {
                IsSuccess = false,
                LogEntries = new ShaderCompilerLogEntry[]
                {
                    new() { Type = ShaderCompilerLogEntryType.Error, Message = "Cannot start Metal shader compiler process." }
                }
            };
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
        
        if (hasErrors)
        {
            return new ShaderCompilerResult
            {
                IsSuccess = false,
                LogEntries = logList.ToArray()
            };
        }
        
        using var process2 = Process.Start(new ProcessStartInfo 
        { 
            FileName = "xcrun", 
            Arguments = $"metallib {airFilePath} -o {outputFilePath}", 
            RedirectStandardError = true, 
            RedirectStandardOutput = true
        });

        if (process2 == null)
        {
            return new ShaderCompilerResult
            {
                IsSuccess = false,
                LogEntries = new ShaderCompilerLogEntry[]
                {
                    new() { Type = ShaderCompilerLogEntryType.Error, Message = "Cannot start Metal shader compiler process." }
                }
            };
        }

        process2.WaitForExit();

        // TODO: Handle errors too here

        while ((line = process2.StandardError.ReadLine()) != null)
        {
            logList.Add(new() { Type = ShaderCompilerLogEntryType.Error, Message = line });
        }
        
        while ((line = process2.StandardOutput.ReadLine()) != null)
        {
            logList.Add(new() { Type = ShaderCompilerLogEntryType.Message, Message = line });
        }

        var shaderBytecode = File.ReadAllBytes(outputFilePath);

        File.Delete(inputFilePath);
        File.Delete(airFilePath);
        File.Delete(outputFilePath);

        return new ShaderCompilerResult
        {
            IsSuccess = true,
            LogEntries = logList.ToArray(),
            ShaderData = shaderBytecode
        };
    }
}