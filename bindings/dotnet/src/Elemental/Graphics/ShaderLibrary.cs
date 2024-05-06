namespace Elemental.Graphics;

/// <summary>
/// Handle that represents a shader library.
/// </summary>
public readonly record struct ShaderLibrary : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        GraphicsServiceInterop.FreeShaderLibrary(this);
    }
}
