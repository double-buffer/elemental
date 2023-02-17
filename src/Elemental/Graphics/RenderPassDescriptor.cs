namespace Elemental.Graphics;

/// <summary>
/// Describes a render pass to create.
/// </summary>
public readonly record struct RenderPassDescriptor
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public RenderPassDescriptor()
    {
    }

    public RenderPassRenderTarget? RenderTarget0 { get; init; }
}