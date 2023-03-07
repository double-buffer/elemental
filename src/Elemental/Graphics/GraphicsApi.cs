namespace Elemental.Graphics;

/// <summary>
/// Supported Graphics APIs.
/// </summary>
public enum GraphicsApi
{
    /// <summary>
    /// Unknown Graphics API.
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// Direct3D12 Graphics API.
    /// </summary>
    Direct3D12 = 1,

    /// <summary>
    /// Vulkan Graphics API.
    /// </summary>
    Vulkan = 2,

    /// <summary>
    /// Metal Graphics API.
    /// </summary>
    Metal = 3
}