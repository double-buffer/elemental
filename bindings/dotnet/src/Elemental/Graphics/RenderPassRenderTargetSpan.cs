namespace Elemental.Graphics;

public ref struct RenderPassRenderTargetSpan
{
    public in RenderPassRenderTarget Items { get; set; }

    public uint Length { get; set; }
}
