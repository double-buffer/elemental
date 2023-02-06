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
        DeviceId = 0;
    }

    /// <summary>
    /// Gets or sets the diagnostics level when creating the device.
    /// </summary>
    public GraphicsDiagnostics GraphicsDiagnostics { get; init; }

    /// <summary>
    /// Gets or sets a graphics device ID to overwrite the default GPU selection.
    /// </summary>
    public ulong DeviceId { get; init; }
}