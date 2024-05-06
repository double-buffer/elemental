namespace Elemental.Graphics;

/// <summary>
/// Configuration options for graphics initialization.
/// </summary>
public ref struct GraphicsOptions
{
    /// <summary>
    /// Enable debugging features if set to true.
    /// </summary>
    public bool EnableDebugLayer { get; set; }

    /// <summary>
    /// Prefer using Vulkan API if set to true.
    /// </summary>
    public bool PreferVulkan { get; set; }
}
