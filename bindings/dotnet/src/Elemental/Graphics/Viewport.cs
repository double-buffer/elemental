namespace Elemental.Graphics;

/// <summary>
/// Defines a viewport for rendering.
/// </summary>
public record struct Viewport
{
    /// <summary>
    /// X coordinate of the viewport's top left corner.
    /// </summary>
    public float X { get; set; }

    /// <summary>
    /// Y coordinate of the viewport's top left corner.
    /// </summary>
    public float Y { get; set; }

    /// <summary>
    /// Width of the viewport.
    /// </summary>
    public float Width { get; set; }

    /// <summary>
    /// Height of the viewport.
    /// </summary>
    public float Height { get; set; }

    /// <summary>
    /// Minimum depth of the viewport.
    /// </summary>
    public float MinDepth { get; set; }

    /// <summary>
    /// Maximum depth of the viewport.
    /// </summary>
    public float MaxDepth { get; set; }
}
