namespace Elemental;

internal unsafe record struct SpanUnsafe<T> where T : struct
{
    internal nuint Items { get; set; }
    internal int Length { get; set; }
}
