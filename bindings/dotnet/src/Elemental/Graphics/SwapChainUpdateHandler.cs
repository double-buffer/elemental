namespace Elemental.Graphics;

/// <summary>
/// Function pointer type for handling updates to the swap chain.
/// </summary>
[NativeMarshalling(typeof(SwapChainUpdateHandlerMarshaller))]
[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void SwapChainUpdateHandler(SwapChainUpdateParameters updateParameters, ReadOnlySpan<byte> payload);

[CustomMarshaller(typeof(SwapChainUpdateHandler), MarshalMode.ManagedToUnmanagedIn, typeof(SwapChainUpdateHandlerMarshaller))]
internal static unsafe class SwapChainUpdateHandlerMarshaller
{
    internal sealed unsafe record InterceptorEntry
    {
        public required SwapChainUpdateHandler Callback { get; init; }
        public required GCHandle Handle { get; init; }
    }

    private static InterceptorEntry? _interceptorEntry;

    private static unsafe void Interceptor(SwapChainUpdateParameters updateParameters, byte* payload)
    {
        if (_interceptorEntry == null || updateParameters != null || payload != null)
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

        _interceptorEntry.Callback(updateParameters, new ReadOnlySpan<byte>(payload, payloadCounter));
    }

    public static nint ConvertToUnmanaged(SwapChainUpdateHandler managed)
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
