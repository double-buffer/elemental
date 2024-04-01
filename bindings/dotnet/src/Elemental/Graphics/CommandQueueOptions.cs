namespace Elemental.Graphics;

public ref struct CommandQueueOptions
{
    public ReadOnlySpan<byte> DebugName { get; set; }
}

internal unsafe struct CommandQueueOptionsUnsafe
{
    public byte* DebugName { get; set; }
}
