namespace Elemental.Tools;

/// <summary>
/// Represents a shader compiler input that includes shader code, shader stage, and entry point.
/// </summary>
[NativeMarshalling(typeof(ShaderCompilerInputMarshaller))]
public readonly record struct ShaderCompilerInput
{
    /// <summary>
    /// Gets or sets the source code of the shader to compile.
    /// </summary>
    public required string ShaderCode { get; init; }

    /// <summary>
    /// Gets or sets the source code of the shader to compile.
    /// </summary>
    public required ShaderStage Stage { get; init; }

    /// <summary>
    /// Gets or sets the name of the entry point function for the shader.
    /// </summary>
    public required string EntryPoint { get; init; }
    
    /// <summary>
    /// Gets or sets the language of the shaders to compile.
    /// </summary>
    public required ShaderLanguage ShaderLanguage { get; init; }
}

[CustomMarshaller(typeof(ShaderCompilerInput), MarshalMode.Default, typeof(ShaderCompilerInputMarshaller))]
internal static unsafe class ShaderCompilerInputMarshaller
{
    internal readonly struct ShaderCompilerInputUnmanaged
    {
        public byte* ShaderCodePointer { get; init; }
        public ShaderStage Stage { get; init; }
        public byte* EntryPoint { get; init; }
        public ShaderLanguage ShaderLanguage { get; init; }
    }

    public static ShaderCompilerInputUnmanaged ConvertToUnmanaged(ShaderCompilerInput managed)
    {
        return new ShaderCompilerInputUnmanaged()
        {
            ShaderCodePointer = Utf8StringMarshaller.ConvertToUnmanaged(managed.ShaderCode),
            Stage = managed.Stage,
            EntryPoint = Utf8StringMarshaller.ConvertToUnmanaged(managed.EntryPoint),
            ShaderLanguage = managed.ShaderLanguage
        };
    }
    
    public static ShaderCompilerInput ConvertToManaged(ShaderCompilerInputUnmanaged unmanaged)
    {
        throw new NotImplementedException();
    }

    public static void Free(ShaderCompilerInputUnmanaged unmanaged)
    {
        Utf8StringMarshaller.Free(unmanaged.ShaderCodePointer);
        Utf8StringMarshaller.Free(unmanaged.EntryPoint);
    }
}