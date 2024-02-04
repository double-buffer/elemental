namespace Elemental.Graphics;

/// <summary>
/// Describes options for <see cref="GraphicsService" /> creation.
/// </summary>
public readonly record struct GraphicsServiceOptions
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public GraphicsServiceOptions()
    {
        GraphicsDiagnostics = GraphicsDiagnostics.None;
    }

    /// <summary>
    /// Gets or sets the diagnostics level when creating the device.
    /// </summary>
    public GraphicsDiagnostics GraphicsDiagnostics { get; init; }
}