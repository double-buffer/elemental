namespace Elemental.Graphics;

/// <summary>
/// Options for creating a command list.
/// </summary>
public ref struct CommandListOptions
{
    /// <summary>
    /// Optional debug name for the command list.
    /// </summary>
    public ReadOnlySpan<byte> DebugName { get; set; }
}

internal unsafe struct CommandListOptionsUnsafe
{
    public byte* DebugName { get; set; }
}

