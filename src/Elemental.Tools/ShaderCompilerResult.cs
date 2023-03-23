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
    public required ToolsShaderStage Stage { get; init; }

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
        public ToolsShaderStage Stage { get; init; }
        public byte* EntryPoint { get; init; }
        public void* ShaderDataPointer { get; init; }
        public int ShaderDataCount { get; init; }
        public ShaderCompilerLogEntryUnmanaged* LogEntryPointer { get; init; }
        public int LogEntryCount { get; init; }
    }

    public static ShaderCompilerResultUnmanaged ConvertToUnmanaged(ShaderCompilerResult managed)
    {
        throw new NotImplementedException();
    }
    
    public static ShaderCompilerResult ConvertToManaged(ShaderCompilerResultUnmanaged unmanaged)
    {
        // TODO: Can we avoid the copy here?
        var shaderData = unmanaged.ShaderDataPointer != null ? new byte[unmanaged.ShaderDataCount] : Array.Empty<byte>();
        var sourceShaderDataSpan = new Span<byte>(unmanaged.ShaderDataPointer, unmanaged.ShaderDataCount);
        sourceShaderDataSpan.CopyTo(shaderData);
        
        var logEntries = unmanaged.LogEntryPointer != null ? new ShaderCompilerLogEntry[unmanaged.LogEntryCount] : Array.Empty<ShaderCompilerLogEntry>();
        var sourceLogEntriesSpan = new Span<ShaderCompilerLogEntryUnmanaged>(unmanaged.LogEntryPointer, unmanaged.LogEntryCount);

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
            LogEntries = logEntries
        };
    }

    public static void Free(ShaderCompilerResultUnmanaged unmanaged)
    {
        var sourceLogEntriesSpan = new Span<ShaderCompilerLogEntryUnmanaged>(unmanaged.LogEntryPointer, unmanaged.LogEntryCount);

        for (var i = 0; i < unmanaged.LogEntryCount; i++)
        {
            var sourceLogEntry = sourceLogEntriesSpan[i];
            PlatformServiceInterop.Native_FreeNativePointer((nint)sourceLogEntry.Message);
        }

        PlatformServiceInterop.Native_FreeNativePointer((nint)unmanaged.ShaderDataPointer);
        PlatformServiceInterop.Native_FreeNativePointer((nint)unmanaged.LogEntryPointer);
    }
}