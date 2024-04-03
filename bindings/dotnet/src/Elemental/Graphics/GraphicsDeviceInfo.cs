namespace Elemental.Graphics;

public record struct GraphicsDeviceInfo
{
    public ReadOnlySpan<byte> DeviceName { get; set; }

    public GraphicsApi GraphicsApi { get; set; }

    public UInt64 DeviceId { get; set; }

    public UInt64 AvailableMemory { get; set; }
}

internal unsafe struct GraphicsDeviceInfoUnsafe
{
    public byte* DeviceName { get; set; }

    public GraphicsApi GraphicsApi { get; set; }

    public UInt64 DeviceId { get; set; }

    public UInt64 AvailableMemory { get; set; }
}

