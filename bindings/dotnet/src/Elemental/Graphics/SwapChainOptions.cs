namespace Elemental.Graphics;

public ref struct SwapChainOptions
{
    public uint Width { get; set; }

    public uint Height { get; set; }

    public SwapChainFormat Format { get; set; }

    public uint MaximumFrameLatency { get; set; }
}
