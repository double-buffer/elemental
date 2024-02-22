namespace Elemental.Graphics;

/// <summary>
/// Handle that represents a graphics device.
/// </summary>
public readonly record struct GraphicsDevice : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        GraphicsServiceInterop.FreeGraphicsDevice(this);
    }
}
