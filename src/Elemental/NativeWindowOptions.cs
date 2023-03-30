namespace Elemental;

/// <summary>
/// Describes a <see cref="NativeWindow" /> to create.
/// </summary>
[NativeMarshalling(typeof(NativeWindowOptionsMarshaller))]
public readonly record struct NativeWindowOptions
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public NativeWindowOptions()
    {
        Title = "Elemental Window";
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

[CustomMarshaller(typeof(NativeWindowOptions), MarshalMode.ManagedToUnmanagedIn, typeof(NativeWindowOptionsMarshaller))]
internal static unsafe class NativeWindowOptionsMarshaller
{
    internal struct NativeWindowOptionsUnmanaged
    {
        public byte* Title;
        public int Width;
        public int Height;
        public NativeWindowState WindowState;
    }

    public static NativeWindowOptionsUnmanaged ConvertToUnmanaged(NativeWindowOptions managed)
    {
        return new NativeWindowOptionsUnmanaged
        {
            Title = Utf8StringMarshaller.ConvertToUnmanaged(managed.Title),
            Width = managed.Width,
            Height = managed.Height,
            WindowState = managed.WindowState
        };
    }

    public static void Free(NativeWindowOptionsUnmanaged unmanaged) => Utf8StringMarshaller.Free(unmanaged.Title);
}