namespace Elemental.Graphics;

public ref struct CommandListOptions
{
    public ReadOnlySpan<byte> DebugName { get; set; }
}

internal unsafe struct CommandListOptionsUnsafe
{
    public byte* DebugName { get; set; }
}

