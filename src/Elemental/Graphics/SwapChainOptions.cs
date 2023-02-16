namespace Elemental.Graphics;

/// <summary>
/// Describes a <see cref="SwapChain" /> to create.
/// </summary>
public readonly record struct SwapChainOptions
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public SwapChainOptions()
    {
        Width = 0;
        Height = 0;
        TextureFormat = TextureFormat.Rgba8UnormSrgb;
    }

    /// <summary>
    /// Width in pixels of the <see cref="SwapChain" />.
    /// </summary>
    /// <value>Width in pixels.</value>
    public int Width { get; init; }
    
    /// <summary>
    /// Height in pixels of the <see cref="SwapChain" />.
    /// </summary>
    /// <value>Height in pixels.</value>
    public int Height { get; init; }

    /// <summary>
    /// Texture format of the swap chain.
    /// </summary>
    /// <value>Texture format of the swap chain.</value>
    public TextureFormat TextureFormat { get; init; }
}