namespace Elemental;

/// <summary>
/// Contains information about the size and scale of a window.
/// </summary>
public ref struct WindowSize
{
    /// <summary>
    /// Width of the window's render area in pixels.
    /// </summary>
    public uint Width { get; set; }

    /// <summary>
    /// Height of the window's render area in pixels.
    /// </summary>
    public uint Height { get; set; }

    /// <summary>
    /// Scale factor for the UI, useful for DPI adjustments.
    /// </summary>
    public float UIScale { get; set; }

    /// <summary>
    /// Current state of the window.
    /// </summary>
    public WindowState WindowState { get; set; }
}
