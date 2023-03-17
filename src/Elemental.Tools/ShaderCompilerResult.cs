namespace Elemental.Tools;

public readonly record struct ShaderCompilerResult
{
    public required bool IsSuccess { get; init; }
    public ReadOnlyMemory<byte> ShaderData { get; init; }
    public ReadOnlyMemory<ShaderCompilerLogEntry> LogEntries { get; init; }

    public static ShaderCompilerResult CreateErrorResult(string message)
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