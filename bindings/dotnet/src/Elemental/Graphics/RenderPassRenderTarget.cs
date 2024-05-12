namespace Elemental.Graphics;

/// <summary>
/// Configuration for a render pass target.
/// </summary>
public record struct RenderPassRenderTarget
{
    /// <summary>
    /// Render target texture.
    /// </summary>
    public Texture RenderTarget { get; set; }

    /// <summary>
    /// Color to clear the render target with if the load action is clear.
    /// </summary>
    public Color ClearColor { get; set; }

    /// <summary>
    /// Action to take when loading data into the render target at the beginning of a render pass.
    /// </summary>
    public RenderPassLoadAction LoadAction { get; set; }

    /// <summary>
    /// Action to take when storing data from the render target at the end of a render pass.
    /// </summary>
    public RenderPassStoreAction StoreAction { get; set; }
}
