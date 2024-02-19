namespace Elemental;

/// <summary>
/// Defines a function pointer type for log handling.
/// </summary>
/// <param name="messageType">The type of the log message.</param>
/// <param name="category">The category of the log message.</param>
/// <param name="function">The function where the log was triggered.</param>
/// <param name="message">The log message.</param>
[NativeMarshalling(typeof(LogHandlerMarshaller))]
[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void LogHandler(LogMessageType messageType, LogMessageCategory category, ReadOnlySpan<byte> function, ReadOnlySpan<byte> message);

[CustomMarshaller(typeof(LogHandler), MarshalMode.ManagedToUnmanagedIn, typeof(LogHandlerMarshaller))]
internal static unsafe class LogHandlerMarshaller
{
    internal sealed unsafe record InterceptorEntry
    {
        public required LogHandler Callback { get; init; }
        public required GCHandle Handle { get; init; }
    }

    private static InterceptorEntry? _interceptorEntry;

    private static unsafe void Interceptor(LogMessageType messageType, LogMessageCategory category, byte* function, byte* message)
    {
        if (_interceptorEntry == null || function == null || message == null)
        {
            return;
        }

        var functionCounter = 0;
        var functionPointer = (byte*)function;

        while (functionPointer[functionCounter] != 0)
        {
            functionCounter++;
        }

        functionCounter++;

        var messageCounter = 0;
        var messagePointer = (byte*)message;

        while (messagePointer[messageCounter] != 0)
        {
            messageCounter++;
        }

        messageCounter++;

        _interceptorEntry.Callback(messageType, category, new ReadOnlySpan<byte>(function, functionCounter), new ReadOnlySpan<byte>(message, messageCounter));
    }

    public static nint ConvertToUnmanaged(LogHandler managed)
    {
        // TODO: Unallocate handle
        var interceptorDelegate = Interceptor;
        var handle = GCHandle.Alloc(interceptorDelegate);
        var unmanaged = Marshal.GetFunctionPointerForDelegate(interceptorDelegate);

        _interceptorEntry = new InterceptorEntry { Callback = managed, Handle = handle };
        return unmanaged;
    }

    public static void Free(nint _)
    {
    }
}
