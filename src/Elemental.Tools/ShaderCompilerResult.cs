namespace Elemental.Tools;

/// <summary>
/// Structure that represents a shader compiler result.
/// </summary>
public readonly record struct ShaderCompilerResult
{
    /// <summary>
    /// Gets a value indicating if the compilation was successful.
    /// </summary>
    /// <value>True if the compilation was successful; Otherwise, false.</value>
    public required bool IsSuccess { get; init; }

    /// <summary>
    /// Gets the compiled shader data.
    /// </summary>
    /// <value>Compiled shader data.</value>
    public ReadOnlyMemory<byte> ShaderData { get; init; }
    
    /// <summary>
    /// Gets the log entries associated with the compilation.
    /// </summary>
    /// <value>Log entries of the compilation.</value>
    public ReadOnlyMemory<ShaderCompilerLogEntry> LogEntries { get; init; }

    internal static ShaderCompilerResult CreateErrorResult(string message)
    {
        return new ShaderCompilerResult
        {
            IsSuccess = false,
            LogEntries = new ShaderCompilerLogEntry[]
            {
                new() { Type = ShaderCompilerLogEntryType.Error, Message = message }
            }
        };
    }
}