namespace Elemental;

/// <summary>
/// Defines a function pointer type for handling application events.
/// </summary>
[NativeMarshalling(typeof(ApplicationHandlerMarshaller<>))]
public delegate void ApplicationHandler<T>(ref T payload) where T : unmanaged;

[CustomMarshaller(typeof(ApplicationHandler<>), MarshalMode.ManagedToUnmanagedIn, typeof(ApplicationHandlerMarshaller<>))]
internal static unsafe class ApplicationHandlerMarshaller<T> where T : unmanaged
{
    internal sealed unsafe record InterceptorEntry
    {
        public required ApplicationHandler<T> Callback { get; init; }
    }

    private static InterceptorEntry? _interceptorEntry;

    private static void Interceptor(ref T payload)
    {
        if (_interceptorEntry == null)
        {
            return;
        }

        _interceptorEntry.Callback(ref payload);
    }

    public static nint ConvertToUnmanaged(ApplicationHandler<T> managed)
    {
        // TODO: Try to avoid all of that
        delegate* <ref T, void> unmanaged = &Interceptor;

        _interceptorEntry = new InterceptorEntry { Callback = managed };
        return (nint)unmanaged;
    }

    public static void Free(nint _)
    {
    }
}
