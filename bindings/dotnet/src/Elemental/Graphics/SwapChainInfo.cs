namespace Elemental.Graphics;

/// <summary>
/// Information about the swap chain setup.
/// </summary>
public record struct SwapChainInfo
{
    /// <summary>
    /// Width of the swap chain in pixels.
    /// </summary>
    public uint Width { get; set; }

    /// <summary>
    /// Height of the swap chain in pixels.
    /// </summary>
    public uint Height { get; set; }

    /// <summary>
    /// Aspect ratio of the swap chain.
    /// </summary>
    public float AspectRatio { get; set; }

    /// <summary>
    /// Format of the textures used in the swap chain.
    /// </summary>
    public TextureFormat Format { get; set; }
}
