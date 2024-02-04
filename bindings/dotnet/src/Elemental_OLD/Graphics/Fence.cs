namespace Elemental.Graphics;

/// <summary>
/// Fence is used to synchronize command queues.
/// </summary>
public readonly record struct Fence
{
    /// <summary>
    /// <see cref="CommandQueue" /> associated with the fence.
    /// </summary>
    /// <value></value>
    public CommandQueue CommandQueue { get; init; }

    /// <summary>
    /// Internal fence value.
    /// </summary>
    /// <value>Internal fence value.</value>
    public ulong FenceValue { get; init; }
}