namespace Elemental;

[NativeMarshalling(typeof(NativeWindowDescriptionMarshaller))]
public readonly record struct NativeWindowDescription
{
    public NativeWindowDescription()
    {
        Title = "Window Title";
        Width = 1280;
        Height = 720;
        IsDpiAware = true;
        WindowState = NativeWindowState.Normal;
    }

    public string Title { get; init; }
    public int Width { get; init; }
    public int Height { get; init; }
    public bool IsDpiAware { get; init; }
    public NativeWindowState WindowState { get; init; }
}

[CustomMarshaller(typeof(NativeWindowDescription), MarshalMode.ManagedToUnmanagedIn, typeof(NativeWindowDescriptionMarshaller))]
internal static unsafe class NativeWindowDescriptionMarshaller
{
    internal struct NativeWindowDescriptionUnmanaged
    {
        public byte* Title;
        public int Width;
        public int Height;
        public bool IsDpiAware;
        public NativeWindowState WindowState;
    }

    public static NativeWindowDescriptionUnmanaged ConvertToUnmanaged(NativeWindowDescription managed)
    {
        return new NativeWindowDescriptionUnmanaged
        {
            Title = Utf8StringMarshaller.ConvertToUnmanaged(managed.Title),
            Width = managed.Width,
            Height = managed.Height,
            IsDpiAware = managed.IsDpiAware,
            WindowState = managed.WindowState
        };
    }

    public static void Free(NativeWindowDescriptionUnmanaged unmanaged)
        => Utf8StringMarshaller.Free(unmanaged.Title);
}