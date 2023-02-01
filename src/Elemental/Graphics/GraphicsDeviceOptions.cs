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
        UseVulkan = false;
    }

    /// <summary>
    /// Gets or sets the diagnostics level when creating the device.
    /// </summary>
    public GraphicsDiagnostics GraphicsDiagnostics { get; init; }

    /// <summary>
    /// Gets or sets a boolean value to use Vulkan (if available on the system) instead of the
    /// prefered native system graphics api.
    /// </summary>
    public bool UseVulkan { get; init; }
}