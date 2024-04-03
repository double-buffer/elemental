namespace Elemental;

/// <summary>
/// Specifies options for window creation.
/// </summary>
public ref struct WindowOptions
{
    /// <summary>
    /// Title of the window.
    /// </summary>
    public ReadOnlySpan<byte> Title { get; set; }

    /// <summary>
    /// Width of the window in pixels.
    /// </summary>
    public uint Width { get; set; }

    /// <summary>
    /// Height of the window in pixels.
    /// </summary>
    public uint Height { get; set; }

    /// <summary>
    /// Initial state of the window.
    /// </summary>
    public WindowState WindowState { get; set; }
}

internal unsafe struct WindowOptionsUnsafe
{
    public byte* Title { get; set; }

    public uint Width { get; set; }

    public uint Height { get; set; }

    public WindowState WindowState { get; set; }
}

