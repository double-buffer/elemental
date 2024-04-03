namespace Elemental.Graphics;

public record struct RenderPassRenderTarget
{
    public Texture RenderTarget { get; set; }

    public Color ClearColor { get; set; }

    public RenderPassLoadAction LoadAction { get; set; }

    public RenderPassStoreAction StoreAction { get; set; }
}
