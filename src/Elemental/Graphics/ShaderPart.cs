namespace Elemental.Graphics;

[NativeMarshalling(typeof(ShaderPartMarshaller))]
public readonly record struct ShaderPart
{
    public required ShaderStage Stage { get; init; }
    public required ReadOnlyMemory<byte> Data { get; init; }
}

[CustomMarshaller(typeof(ShaderPart), MarshalMode.Default, typeof(ShaderPartMarshaller))]
internal static unsafe class ShaderPartMarshaller
{
    internal readonly struct ShaderPartUnmanaged
    {
        public ShaderStage Stage { get; init; }
        public void* DataPointer { get; init; }
        public int DataCount { get; init; }
    }

    public static ShaderPartUnmanaged ConvertToUnmanaged(ShaderPart managed)
    {
        // TODO: Can we avoid the copy here?
        var dataPointer = NativeMemory.Alloc((nuint)managed.Data.Length);
        var dataSpan = new Span<byte>(dataPointer, managed.Data.Length);
        managed.Data.Span.CopyTo(dataSpan);

        return new ShaderPartUnmanaged
        {
            Stage = managed.Stage,
            DataPointer = dataPointer,
            DataCount = managed.Data.Length
        };
    }
    
    public static ShaderPart ConvertToManaged(ShaderPartUnmanaged _)
    {
        throw new NotImplementedException();
    }

    public static void Free(ShaderPartUnmanaged unmanaged)
    {
        NativeMemory.Free(unmanaged.DataPointer);
    }
}