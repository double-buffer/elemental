namespace Elemental;

/// <summary>
/// Describes a <see cref="NativeWindow" /> to create.
/// </summary>
[NativeMarshalling(typeof(NativeWindowDescriptionMarshaller))]
public readonly record struct NativeWindowDescription
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public NativeWindowDescription()
    {
        Title = "Window Title";
        Width = 1280;
        Height = 720;
        WindowState = NativeWindowState.Normal;
    }

    /// <summary>
    /// Gets or sets the title of the window.
    /// </summary>
    /// <value>Title of the window.</value>
    public string Title { get; init; }
    
    /// <summary>
    /// Gets or sets the width of the window.
    /// </summary>
    /// <value>Width of the window.</value>
    public int Width { get; init; }
    
    /// <summary>
    /// Gets or sets the height of the window.
    /// </summary>
    /// <value>Height of the window.</value>
    public int Height { get; init; }

    /// <summary>
    /// Gets or sets the initial state of the window.
    /// </summary>
    /// <value>State of the window.</value> 
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
        public NativeWindowState WindowState;
    }

    public static NativeWindowDescriptionUnmanaged ConvertToUnmanaged(NativeWindowDescription managed)
    {
        return new NativeWindowDescriptionUnmanaged
        {
            Title = Utf8StringMarshaller.ConvertToUnmanaged(managed.Title),
            Width = managed.Width,
            Height = managed.Height,
            WindowState = managed.WindowState
        };
    }

    public static void Free(NativeWindowDescriptionUnmanaged unmanaged)
        => Utf8StringMarshaller.Free(unmanaged.Title);
}