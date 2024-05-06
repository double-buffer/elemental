namespace Elemental.Graphics;

/// <summary>
/// Handle that represents a command queue.
/// </summary>
public readonly record struct CommandQueue : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        GraphicsServiceInterop.FreeCommandQueue(this);
    }
}
