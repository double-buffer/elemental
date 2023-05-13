namespace Elemental;

/// <summary>
/// Describes a <see cref="NativeApplication" /> to create.
/// </summary>
[NativeMarshalling(typeof(NativeApplicationOptionsMarshaller))]
public readonly record struct NativeApplicationOptions
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public NativeApplicationOptions()
    {
    }

    /// <summary>
    /// Gets or sets an handle that will receive debug messages.
    /// </summary>
    /// <value>Log message handler.</value>
    public LogMessageHandler? LogMessageHandler { get; init; }
}

[CustomMarshaller(typeof(NativeApplicationOptions), MarshalMode.ManagedToUnmanagedIn, typeof(NativeApplicationOptionsMarshaller))]
internal static unsafe class NativeApplicationOptionsMarshaller
{
    internal struct NativeApplicationOptionsUnmanaged
    {
        public nint LogMessageHandler;
    }

    public static NativeApplicationOptionsUnmanaged ConvertToUnmanaged(NativeApplicationOptions managed)
    {
        return new NativeApplicationOptionsUnmanaged
        {
            LogMessageHandler = managed.LogMessageHandler != null ? LogMessageHandlerMarshaller.ConvertToUnmanaged(managed.LogMessageHandler) : nint.Zero
        };
    }

    public static void Free(NativeApplicationOptionsUnmanaged _)
    {
    }
}