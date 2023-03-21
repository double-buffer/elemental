namespace Elemental.Graphics;

/// <summary>
/// Represents a part of a Shader.
/// </summary>
[NativeMarshalling(typeof(ShaderPartMarshaller))]
public readonly record struct ShaderPart
{
    /// <summary>
    /// Gets or sets the stage of the shader part.
    /// </summary>
    /// <value>Stage of the shader part.</value>
    public required ShaderStage Stage { get; init; }

    /// <summary>
    /// Gets or sets the entry point function name of the shader part.
    /// </summary>
    /// <value>Entry point function name.</value>
    public required string EntryPoint { get; init; }

    /// <summary>
    /// Gets or sets the compiled shader.
    /// The binary data is platform specific. It can be provided by 
    /// manually using the tools of the platform or by using Elemental.Tools.
    /// </summary>
    /// <value>Compiled shader.</value>
    public required ReadOnlyMemory<byte> Data { get; init; }

    /// <summary>
    /// Gets or sets meta data associated with the shader part. This metadata will
    /// be used by the different platforms to complete the pipeline creation if 
    /// some informations are missing.
    /// </summary>
    /// <value>List of metadata values.</value>
    public ReadOnlyMemory<ShaderPartMetaData> MetaData { get; init; }
}

[CustomMarshaller(typeof(ShaderPart), MarshalMode.Default, typeof(ShaderPartMarshaller))]
internal static unsafe class ShaderPartMarshaller
{
    internal readonly struct ShaderPartUnmanaged
    {
        public ShaderStage Stage { get; init; }
        public byte* EntryPoint { get; init; }
        public void* DataPointer { get; init; }
        public int DataCount { get; init; }
        public ShaderPartMetaData* MetaDataPointer { get; init; }
        public int MetaDataCount { get; init; }
    }

    public static ShaderPartUnmanaged ConvertToUnmanaged(ShaderPart managed)
    {
        // TODO: Can we avoid the copy here?
        var dataPointer = NativeMemory.Alloc((nuint)managed.Data.Length);
        var dataSpan = new Span<byte>(dataPointer, managed.Data.Length);
        managed.Data.Span.CopyTo(dataSpan);
        
        var metaDataPointer = (ShaderPartMetaData*)NativeMemory.Alloc((nuint)(managed.MetaData.Length * Marshal.SizeOf<ShaderPartMetaData>()));
        var metaDataSpan = new Span<ShaderPartMetaData>(metaDataPointer, managed.MetaData.Length);
        managed.MetaData.Span.CopyTo(metaDataSpan);

        return new ShaderPartUnmanaged
        {
            Stage = managed.Stage,
            EntryPoint = Utf8StringMarshaller.ConvertToUnmanaged(managed.EntryPoint),
            DataPointer = dataPointer,
            DataCount = managed.Data.Length,
            MetaDataPointer = metaDataPointer,
            MetaDataCount = managed.MetaData.Length
        };
    }
    
    public static ShaderPart ConvertToManaged(ShaderPartUnmanaged _)
    {
        throw new NotImplementedException();
    }

    public static void Free(ShaderPartUnmanaged unmanaged)
    {
        Utf8StringMarshaller.Free(unmanaged.EntryPoint);
        NativeMemory.Free(unmanaged.DataPointer);
        NativeMemory.Free(unmanaged.MetaDataPointer);
    }
}