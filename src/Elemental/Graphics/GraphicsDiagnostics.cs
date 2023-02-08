namespace Elemental.Graphics;

/// <summary>
/// Diagnostics levels used by <see cref="IGraphicsService" />.
/// </summary>
public enum GraphicsDiagnostics
{
    /// <summary>
    /// No diagnostics. This is the default and should be set for release mode.
    /// </summary>
    None = 0,
    
    /// <summary>
    /// Debug diagnostics. This enables the graphics API validation layer.
    /// It also enable the debug names for each created resources.
    /// </summary>
    Debug = 1
}