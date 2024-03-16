namespace Elemental.Graphics;

public ref struct GraphicsCommandQueueOptions
{
    public ReadOnlySpan<byte> DebugName { get; set; }
}

internal unsafe struct GraphicsCommandQueueOptionsUnsafe
{
    public byte* DebugName { get; set; }
}
