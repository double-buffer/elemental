namespace Elemental;

public ref struct WindowOptions
{
    public ReadOnlySpan<byte> Title { get; set; }

    public uint Width { get; set; }

    public uint Height { get; set; }

    public WindowState WindowState { get; set; }
}

internal unsafe struct WindowOptionsUnsafe
{
    public byte* Title { get; set; }

    public uint Width { get; set; }

    public uint Height { get; set; }

    public WindowState WindowState { get; set; }
}
