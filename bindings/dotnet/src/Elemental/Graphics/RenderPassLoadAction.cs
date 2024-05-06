namespace Elemental.Graphics;

/// <summary>
/// Enumerates render pass load actions.
/// </summary>
public enum RenderPassLoadAction
{
    /// <summary>
    /// Discards the previous contents.
    /// </summary>
    Discard = 0,

    /// <summary>
    /// Loads the existing contents.
    /// </summary>
    Load = 1,

    /// <summary>
    /// Clears to a predefined value.
    /// </summary>
    Clear = 2
}
