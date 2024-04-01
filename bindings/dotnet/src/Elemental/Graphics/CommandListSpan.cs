namespace Elemental.Graphics;

public ref struct CommandListSpan
{
    public in CommandList Items { get; set; }

    public uint Length { get; set; }
}
