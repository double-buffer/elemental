namespace Elemental;

/// <summary>
/// Defines a function pointer type for handling application events.
/// </summary>
[NativeMarshalling(typeof(ApplicationHandlerMarshaller))]
[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void ApplicationHandler(ReadOnlySpan<byte> payload);

[CustomMarshaller(typeof(ApplicationHandler), MarshalMode.ManagedToUnmanagedIn, typeof(ApplicationHandlerMarshaller))]
internal static unsafe class ApplicationHandlerMarshaller
{
    internal sealed unsafe record InterceptorEntry
    {
        public required ApplicationHandler Callback { get; init; }
        public required GCHandle Handle { get; init; }
    }

    private static InterceptorEntry? _interceptorEntry;

    private static unsafe void Interceptor(byte* payload)
    {
        if (_interceptorEntry == null || payload != null)
        {
            return;
        }

        var payloadCounter = 0;
        var payloadPointer = (byte*)payload;

        while (payloadPointer[payloadCounter] != 0)
        {
            payloadCounter++;
        }

        payloadCounter++;

        _interceptorEntry.Callback(new ReadOnlySpan<byte>(payload, payloadCounter));
    }

    public static nint ConvertToUnmanaged(ApplicationHandler managed)
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
