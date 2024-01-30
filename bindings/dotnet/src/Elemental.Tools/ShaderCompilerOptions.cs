namespace Elemental.Tools;

/// <summary>
/// Describes a <see cref="ShaderCompiler" /> to create.
/// </summary>
[NativeMarshalling(typeof(ShaderCompilerOptionsMarshaller))]
public readonly record struct ShaderCompilerOptions
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public ShaderCompilerOptions()
    {
    }

    /// <summary>
    /// Gets or sets an handle that will receive debug messages.
    /// </summary>
    /// <value>Log message handler.</value>
    public LogMessageHandler? LogMessageHandler { get; init; }
}

[CustomMarshaller(typeof(ShaderCompilerOptions), MarshalMode.ManagedToUnmanagedIn, typeof(ShaderCompilerOptionsMarshaller))]
internal static unsafe class ShaderCompilerOptionsMarshaller
{
    internal struct ShaderCompilerOptionsUnmanaged
    {
        public nint LogMessageHandler;
    }

    public static ShaderCompilerOptionsUnmanaged ConvertToUnmanaged(ShaderCompilerOptions managed)
    {
        return new ShaderCompilerOptionsUnmanaged
        {
            LogMessageHandler = managed.LogMessageHandler != null ? LogMessageHandlerMarshaller.ConvertToUnmanaged(managed.LogMessageHandler) : nint.Zero
        };
    }

    public static void Free(ShaderCompilerOptionsUnmanaged _)
    {
    }
}