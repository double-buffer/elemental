namespace Elemental.Graphics;

public ref struct SwapChainInfo
{
    public uint Width { get; set; }

    public uint Height { get; set; }

    public TextureFormat Format { get; set; }
}
