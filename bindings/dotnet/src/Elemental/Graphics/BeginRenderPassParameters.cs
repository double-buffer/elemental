namespace Elemental.Graphics;

public ref struct BeginRenderPassParameters
{
    public ReadOnlyMemory<RenderPassRenderTarget> RenderTargets { get; set; }
}

internal unsafe struct BeginRenderPassParametersUnsafe
{
    public RenderPassRenderTarget* RenderTargets { get; set; }
}

