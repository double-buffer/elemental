namespace Elemental.Tools;

/// <summary>
/// Enumerates the type of <see cref="ShaderCompilerLogEntry" />.
/// </summary>
public enum ShaderCompilerLogEntryType
{
    /// <summary>
    /// Message log type.
    /// </summary>
    Message = 0,
    
    /// <summary>
    /// Warning log type.
    /// </summary>
    Warning = 1,
    
    /// <summary>
    /// Error log type.
    /// </summary>
    Error = 2
}
