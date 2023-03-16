namespace Elemental.Tools;

public readonly record struct ShaderCompilerLogEntry
{
    public required ShaderCompilerLogEntryType Type { get; init; }
    public required string Message { get; init; }
}