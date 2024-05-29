namespace Elemental.Graphics;

/// <summary>
/// Parameters for beginning a render pass.
/// </summary>
public ref struct BeginRenderPassParameters
{
    /// <summary>
    /// Render targets to be used in the render pass.
    /// </summary>
    public ReadOnlySpan<RenderPassRenderTarget> RenderTargets { get; set; }

    /// <summary>
    /// Viewports to be used in the render pass.
    /// </summary>
    public ReadOnlySpan<Viewport> Viewports { get; set; }
}

internal unsafe struct BeginRenderPassParametersUnsafe
{
    public RenderPassRenderTarget* RenderTargets { get; set; }

    public Viewport* Viewports { get; set; }
}

