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
    
    /// <summary>
    /// Gets or sets the render target 1 properties.
    /// </summary>
    /// <value>Render target 1 properties.</value>
    public RenderPassRenderTarget? RenderTarget1 { get; init; }
    
    /// <summary>
    /// Gets or sets the render target 2 properties.
    /// </summary>
    /// <value>Render target 2 properties.</value>
    public RenderPassRenderTarget? RenderTarget2 { get; init; }
    
    /// <summary>
    /// Gets or sets the render target 3 properties.
    /// </summary>
    /// <value>Render target 3 properties.</value>
    public RenderPassRenderTarget? RenderTarget3 { get; init; }

    // TODO: Check max render target count
}