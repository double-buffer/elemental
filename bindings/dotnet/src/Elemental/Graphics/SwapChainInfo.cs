namespace Elemental.Graphics;

public record struct SwapChainInfo
{
    public uint Width { get; set; }

    public uint Height { get; set; }

    public TextureFormat Format { get; set; }
}
