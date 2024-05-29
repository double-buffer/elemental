namespace Elemental.Inputs;

public record struct InputStream
{
    public ReadOnlySpan<InputEvent> Events { get; set; }
}

internal unsafe struct InputStreamUnsafe
{
    public InputEvent* Events { get; set; }
}

