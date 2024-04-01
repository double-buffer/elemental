namespace Elemental.Graphics;

public ref struct Fence
{
    public CommandQueue CommandQueue { get; set; }

    public UInt64 FenceValue { get; set; }
}
