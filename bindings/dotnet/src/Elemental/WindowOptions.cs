namespace Elemental;

/// <summary>
/// Defines options for creating a new window.
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

    /// <summary>
    /// True if the cursor should be hidden.
    /// </summary>
    public bool IsCursorHidden { get; set; }
}

internal unsafe struct WindowOptionsUnsafe
{
    public byte* Title { get; set; }

    public uint Width { get; set; }

    public uint Height { get; set; }

    public WindowState WindowState { get; set; }

    public bool IsCursorHidden { get; set; }
}

