namespace Elemental.Graphics;

/// <summary>
/// Options for creating a graphics device.
/// </summary>
public ref struct GraphicsDeviceOptions
{
    /// <summary>
    /// Identifier for a specific device to be initialized.
    /// </summary>
    public UInt64 DeviceId { get; set; }
}
