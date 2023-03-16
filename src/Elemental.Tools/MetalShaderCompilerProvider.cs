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
}