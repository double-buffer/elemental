namespace Elemental.Graphics;

/// <summary>
/// Enumerates render pass store actions.
/// </summary>
public enum RenderPassStoreAction
{
    /// <summary>
    /// Preserves the contents after rendering.
    /// </summary>
    Store = 0,

    /// <summary>
    /// Discards the contents after rendering.
    /// </summary>
    Discard = 1
}
