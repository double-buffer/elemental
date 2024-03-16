namespace Elemental.Graphics;

public readonly record struct GraphicsCommandQueue : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        GraphicsServiceInterop.FreeGraphicsCommandQueue(this);
    }
}
