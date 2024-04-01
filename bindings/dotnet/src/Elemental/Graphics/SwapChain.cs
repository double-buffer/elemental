namespace Elemental.Graphics;

public readonly record struct SwapChain : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        GraphicsServiceInterop.FreeSwapChain(this);
    }
}
