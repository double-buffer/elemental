namespace Elemental;

public enum LogMessageType
{
    Debug = 0,
    Warning = 1,
    Error = 2
}

public enum LogMessageCategory
{
    NativeApplication,
    Graphics,
    Inputs
}

public delegate void LogMessageHandler(LogMessageType messageType, LogMessageCategory category, string function, string message);

[CustomMarshaller(typeof(LogMessageHandler), MarshalMode.ManagedToUnmanagedIn, typeof(LogMessageHandlerMarshaller))]
public static unsafe class LogMessageHandlerMarshaller
{
    internal unsafe record InterceptorEntry
    {
        public required LogMessageHandler Callback { get; init; }
        public required GCHandle Handle { get; init; }
    }

    private static InterceptorEntry? _interceptorEntry;

    private static unsafe void Interceptor(LogMessageType messageType, LogMessageCategory category, void* function, void* message)
    {
        if (_interceptorEntry == null)
        {
            return;
        }

        _interceptorEntry.Callback(messageType, category, Marshal.PtrToStringUni(new nint(function)) ?? "", Marshal.PtrToStringUni(new nint(message)) ?? "");
    }

    public static nint ConvertToUnmanaged(LogMessageHandler managed)
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