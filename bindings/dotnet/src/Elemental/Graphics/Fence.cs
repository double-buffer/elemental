namespace Elemental.Graphics;

public record struct Fence
{
    public CommandQueue CommandQueue { get; set; }

    public UInt64 FenceValue { get; set; }
}
