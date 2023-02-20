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

    /// <summary>
    /// Gets or sets the render target 0 properties.
    /// </summary>
    /// <value>Render target 0 properties.</value>
    public RenderPassRenderTarget? RenderTarget0 { get; init; }
}