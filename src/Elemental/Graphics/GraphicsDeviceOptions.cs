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
        DeviceId = 0;
    }

    /// <summary>
    /// Gets or sets a graphics device ID to overwrite the default GPU selection.
    /// </summary>
    public ulong DeviceId { get; init; }
}