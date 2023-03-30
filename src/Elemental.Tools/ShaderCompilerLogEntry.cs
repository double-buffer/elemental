namespace Elemental.Tools;

/// <summary>
/// Structure that represents a shader compiler log entry.
/// </summary>
public readonly record struct ShaderCompilerLogEntry
{
    /// <summary>
    /// Gets the type of the log entry.
    /// </summary>
    /// <value>Type of the log entry.</value>
    public required ShaderCompilerLogEntryType Type { get; init; }

    /// <summary>
    /// Gets the message of the log entry.
    /// </summary>
    /// <value>Message of the log entry.</value>
    public required string Message { get; init; }
}