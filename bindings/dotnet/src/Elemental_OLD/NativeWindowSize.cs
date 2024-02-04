namespace Elemental;

/// <summary>
/// Contains size information about a <see cref="NativeWindow" />.
/// </summary>
public readonly record struct NativeWindowSize
{
    /// <summary>
    /// Gets the width of the window.
    /// </summary>
    /// <value>Width of the window.</value>
    public int Width { get; }

    /// <summary>
    /// Gets the height of the window.
    /// </summary>
    /// <value>Height of the window.</value>
    public int Height { get; }

    /// <summary>
    /// Gets the UIScale factor. This value is greater than 1 when the window 
    /// is located on a high DPI screen.
    /// </summary>
    /// <value>UIScale of the window.</value>
    public float UIScale { get; }

    /// <summary>
    /// Gets the Window state.
    /// </summary>
    /// <value>State of the window.</value>
    public NativeWindowState WindowState { get; }
}

