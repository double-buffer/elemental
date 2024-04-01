namespace Elemental.Graphics;

public readonly record struct CommandList : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        GraphicsServiceInterop.FreeCommandList(this);
    }
}
