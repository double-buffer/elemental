namespace Elemental.Graphics;

/// <summary>
/// Information about a graphics device.
/// </summary>
public record struct GraphicsDeviceInfo
{
    /// <summary>
    /// Name of the graphics device.
    /// </summary>
    public ReadOnlySpan<byte> DeviceName { get; set; }

    /// <summary>
    /// API used by the graphics device.
    /// </summary>
    public GraphicsApi GraphicsApi { get; set; }

    /// <summary>
    /// Unique identifier for the device.
    /// </summary>
    public UInt64 DeviceId { get; set; }

    /// <summary>
    /// Available memory on the device.
    /// </summary>
    public UInt64 AvailableMemory { get; set; }
}

internal unsafe struct GraphicsDeviceInfoUnsafe
{
    public byte* DeviceName { get; set; }

    public GraphicsApi GraphicsApi { get; set; }

    public UInt64 DeviceId { get; set; }

    public UInt64 AvailableMemory { get; set; }
}

