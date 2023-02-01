namespace Elemental.Graphics;

/// <summary>
/// Describes a <see cref="GraphicsDevice" /> to create.
/// </summary>
public readonly record struct GraphicsDeviceOptions
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public GraphicsDeviceOptions()
    {
        GraphicsDiagnostics = GraphicsDiagnostics.None;
    }

    /// <summary>
    /// Gets or sets the diagnostics level when creating the device.
    /// </summary>
    public GraphicsDiagnostics GraphicsDiagnostics { get; init; }
}