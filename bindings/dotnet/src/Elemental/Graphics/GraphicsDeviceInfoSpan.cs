namespace Elemental.Graphics;

public ref struct GraphicsDeviceInfoSpan
{
    public in GraphicsDeviceInfo Items { get; set; }

    public uint Length { get; set; }
}
