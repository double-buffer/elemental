namespace Elemental.Tools;

/// <summary>
/// Specifies options for compiling shaders.
/// </summary>
public readonly record struct ShaderCompilationOptions
{
    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderCompilationOptions"/> struct with the default options.
    /// </summary>
    public ShaderCompilationOptions()
    {
        DebugMode = false;
    }

    /// <summary>
    /// Gets or sets a value indicating whether the shader should be compiled in debug mode.
    /// </summary>
    public bool DebugMode { get; init; }

    // TODO: Add specialization constants
}