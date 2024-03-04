namespace Elemental;

[NativeMarshalling(typeof(WindowOptionsMarshaller))]
public ref struct WindowOptions
{
    public ReadOnlySpan<byte> Title { get; set; }

    public int Width { get; set; }

    public int Height { get; set; }

    public WindowState WindowState { get; set; }
}

[CustomMarshaller(typeof(WindowOptions), MarshalMode.ManagedToUnmanagedIn, typeof(WindowOptionsMarshaller))]
internal static unsafe class WindowOptionsMarshaller
{
    internal unsafe struct WindowOptionsUnsafe
    {
        public byte* Title { get; set; }

        public int Width { get; set; }

        public int Height { get; set; }

        public WindowState WindowState { get; set; }
        public nint TitleGCHandle { get; set; }
    }

    public static WindowOptionsUnsafe ConvertToUnmanaged(WindowOptions managed)
    {
        var result = new WindowOptionsUnsafe();
var TitleHandle = GCHandle.Alloc(managed.Title.ToArray(), GCHandleType.Pinned);
result.Title = (byte*)TitleHandle.AddrOfPinnedObject();
result.TitleGCHandle = GCHandle.ToIntPtr(TitleHandle);
        return result;
    }

    public static void Free(WindowOptionsUnsafe _)
    {
    }
}
