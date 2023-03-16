namespace Elemental.Tools;

public readonly record struct ShaderCompilerResult
{
    public required bool IsSuccess { get; init; }
    public ReadOnlyMemory<byte> ShaderData { get; init; }
    public ReadOnlyMemory<ShaderCompilerLogEntry> LogEntries { get; init; }
}