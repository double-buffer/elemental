namespace Elemental.Graphics;

/// <summary>
/// Represents a fence for command list synchronization.
/// </summary>
public record struct Fence
{
    /// <summary>
    /// Associated command queue for the fence.
    /// </summary>
    public CommandQueue CommandQueue { get; set; }

    /// <summary>
    /// The fence value to be reached.
    /// </summary>
    public UInt64 FenceValue { get; set; }
}
