namespace Elemental;

/// <summary>
/// Delegates that can be used to gets log messages.
/// </summary>
/// <param name="messageType">Type of the message.</param>
/// <param name="category">Category of the message.</param>
/// <param name="function"></param>
/// <param name="message"></param>
public delegate void LogMessageHandler(LogMessageType messageType, LogMessageCategory category, string function, string message);

/// <summary>
/// Converts the handler to unmanaged.
/// </summary>
[CustomMarshaller(typeof(LogMessageHandler), MarshalMode.ManagedToUnmanagedIn, typeof(LogMessageHandlerMarshaller))]
public static unsafe class LogMessageHandlerMarshaller
{
    internal sealed unsafe record InterceptorEntry
    {
        public required LogMessageHandler Callback { get; init; }
        public required GCHandle Handle { get; init; }
    }

    private static InterceptorEntry? _interceptorEntry;

    private static unsafe void Interceptor(LogMessageType messageType, LogMessageCategory category, byte* function, byte* message)
    {
        if (_interceptorEntry == null || function == null || message == null)
        {
            return;
        }

        _interceptorEntry.Callback(messageType, category, Utf8StringMarshaller.ConvertToManaged(function) ?? "", Utf8StringMarshaller.ConvertToManaged(message) ?? "");
    }

    /// <summary>
    /// Convert to unmanaged.
    /// </summary>
    /// <param name="managed">Managed version of the object.</param>
    /// <returns>Unmanaged pointer.</returns>
    public static nint ConvertToUnmanaged(LogMessageHandler managed)
    {
        // TODO: Unallocate handle
        var interceptorDelegate = Interceptor;
        var handle = GCHandle.Alloc(interceptorDelegate);
        var unmanaged = Marshal.GetFunctionPointerForDelegate(interceptorDelegate);

        _interceptorEntry = new InterceptorEntry { Callback = managed, Handle = handle };
        return unmanaged;
    }

    /// <summary>
    /// Frees the native pointer.
    /// </summary>
    /// <param name="_">Native pointer.</param>
    public static void Free(nint _)
    {
    }
}
