namespace Elemental.Graphics;

/// <summary>
/// Describes a render pass render target.
/// </summary>
public readonly record struct RenderPassRenderTarget
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public RenderPassRenderTarget()
    {
    }

    /// <summary>
    /// Gets or sets the render target texture.
    /// </summary>
    /// <value>Render target texture.</value>
    public Texture Texture { get; init; }

    // TODO: Additional potential properties: Check the docs first
    // - LoadAction (Discard, Load) clear is implictly declared with the color
    // - StoreAction (Discard, Store)

    // - Those could be store in a textureview?
    // - level
    // - slice
    // - depthplace

    /// <summary>
    /// Gets or sets the clear color if needed.
    /// </summary>
    /// <value>Clear color.</value>
    public Vector4? ClearColor { get; init; }
}