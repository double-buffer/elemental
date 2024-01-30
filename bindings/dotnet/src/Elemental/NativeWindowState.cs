namespace Elemental;

/// <summary>
/// Initial state of a <see cref="NativeWindow" />.
/// </summary>
public enum NativeWindowState
{
    /// <summary>
    /// Normal state. This is the default state.
    /// </summary>
    Normal = 0,

    /// <summary>
    /// Minimized state.
    /// </summary>
    Minimized = 1,
    
    /// <summary>
    /// Maximized state.
    /// </summary>
    Maximized = 2,

    /// <summary>
    /// FullScreen state.
    /// </summary>
    FullScreen = 3
}