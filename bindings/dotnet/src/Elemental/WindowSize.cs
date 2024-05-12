namespace Elemental;

/// <summary>
/// Contains detailed information about the size and scaling factors of a window's render area.
/// </summary>
public record struct WindowSize
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
    /// UI scale factor, typically used for DPI adjustments.
    /// </summary>
    public float UIScale { get; set; }

    /// <summary>
    /// Current state of the window.
    /// </summary>
    public WindowState WindowState { get; set; }
}
