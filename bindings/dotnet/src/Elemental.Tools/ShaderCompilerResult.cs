namespace Elemental.Tools;

/// <summary>
/// Structure that represents a shader compiler result.
/// </summary>
[NativeMarshalling(typeof(ShaderCompilerResultMarshaller))]
public readonly record struct ShaderCompilerResult
{
    /// <summary>
    /// Gets a value indicating if the compilation was successful.
    /// </summary>
    /// <value>True if the compilation was successful; Otherwise, false.</value>
    public required bool IsSuccess { get; init; }

    /// <summary>
    /// Gets the stage to which the shader compiler was compiled.
    /// </summary>
    /// <value>Shader stage.</value>
    public required ShaderStage Stage { get; init; }

    /// <summary>
    /// Gets the entry point that was used to compile the shader.
    /// </summary>
    /// <value>Entry point name.</value>
    public required string EntryPoint { get; init; }

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

    /// <summary>
    /// Gets the meta data values associated with the compilation.
    /// </summary>
    /// <value>Meta data values.</value>
    public ReadOnlyMemory<ShaderMetaData> MetaData { get; init; }
}

[CustomMarshaller(typeof(ShaderCompilerResult), MarshalMode.Default, typeof(ShaderCompilerResultMarshaller))]
internal static unsafe class ShaderCompilerResultMarshaller
{
    internal readonly struct ShaderCompilerLogEntryUnmanaged
    {
        public ShaderCompilerLogEntryType Type { get; init; }
        public byte* Message { get; init; }
    }

    internal readonly struct ShaderCompilerResultUnmanaged
    {
        public bool IsSuccess { get; init; }
        public ShaderStage Stage { get; init; }
        public byte* EntryPoint { get; init; }
        public void* ShaderDataPointer { get; init; }
        public uint ShaderDataCount { get; init; }
        public ShaderCompilerLogEntryUnmanaged* LogEntryPointer { get; init; }
        public uint LogEntryCount { get; init; }
        public ShaderMetaData* MetaDataPointer { get; init; }
        public uint MetaDataCount { get; init; }
    }

    public static ShaderCompilerResultUnmanaged ConvertToUnmanaged(ShaderCompilerResult _)
    {
        throw new NotImplementedException();
    }
    
    public static ShaderCompilerResult ConvertToManaged(ShaderCompilerResultUnmanaged unmanaged)
    {
        // TODO: Avoid the string conversions

        var shaderData = unmanaged.ShaderDataPointer != null ? new byte[unmanaged.ShaderDataCount] : Array.Empty<byte>();
        var sourceShaderDataSpan = new Span<byte>(unmanaged.ShaderDataPointer, (int)unmanaged.ShaderDataCount);
        sourceShaderDataSpan.CopyTo(shaderData);

        var logEntries = unmanaged.LogEntryPointer != null ? new ShaderCompilerLogEntry[unmanaged.LogEntryCount] : Array.Empty<ShaderCompilerLogEntry>();
        var sourceLogEntriesSpan = new Span<ShaderCompilerLogEntryUnmanaged>(unmanaged.LogEntryPointer, (int)unmanaged.LogEntryCount);

        var shaderMetaData = unmanaged.MetaDataPointer != null ? new ShaderMetaData[unmanaged.MetaDataCount] : Array.Empty<ShaderMetaData>();
        var sourceMetaDataSpan = new Span<ShaderMetaData>(unmanaged.MetaDataPointer, (int)unmanaged.MetaDataCount);
        sourceMetaDataSpan.CopyTo(shaderMetaData);

        for (var i = 0; i < unmanaged.LogEntryCount; i++)
        {
            var sourceLogEntry = sourceLogEntriesSpan[i];

            logEntries[i] = new ShaderCompilerLogEntry
            {
                Type = sourceLogEntry.Type,
                     Message = Utf8StringMarshaller.ConvertToManaged(sourceLogEntry.Message) ?? string.Empty
            };
        }

        return new ShaderCompilerResult
        {
            IsSuccess = unmanaged.IsSuccess,
                      Stage = unmanaged.Stage,
                      EntryPoint = Utf8StringMarshaller.ConvertToManaged(unmanaged.EntryPoint) ?? string.Empty,
                      ShaderData = shaderData,
                      LogEntries = logEntries,
                      MetaData = shaderMetaData
        };
    }

    public static void Free(ShaderCompilerResultUnmanaged _)
    {
    }
}
