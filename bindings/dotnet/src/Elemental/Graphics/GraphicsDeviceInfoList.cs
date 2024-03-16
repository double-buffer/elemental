namespace Elemental.Graphics;

public ref struct GraphicsDeviceInfoList
{
    public in GraphicsDeviceInfo Items { get; set; }

    public uint Length { get; set; }
}
