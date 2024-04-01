namespace Elemental.Graphics;

public ref struct FenceSpan
{
    public in Fence Items { get; set; }

    public uint Length { get; set; }
}
