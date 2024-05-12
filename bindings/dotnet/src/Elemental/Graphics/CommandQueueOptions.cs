namespace Elemental.Graphics;

/// <summary>
/// Options for creating a command queue.
/// </summary>
public ref struct CommandQueueOptions
{
    /// <summary>
    /// Optional debug name for the command queue.
    /// </summary>
    public ReadOnlySpan<byte> DebugName { get; set; }
}

internal unsafe struct CommandQueueOptionsUnsafe
{
    public byte* DebugName { get; set; }
}

