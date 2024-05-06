namespace Elemental.Graphics;

/// <summary>
/// Represents RGBA color.
/// </summary>
public record struct Color
{
    /// <summary>
    /// Red component.
    /// </summary>
    public float Red { get; set; }

    /// <summary>
    /// Green component.
    /// </summary>
    public float Green { get; set; }

    /// <summary>
    /// Blue component.
    /// </summary>
    public float Blue { get; set; }

    /// <summary>
    /// Alpha component.
    /// </summary>
    public float Alpha { get; set; }
}
