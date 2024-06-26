namespace Elemental;

/// <summary>
/// Represents a handle to an elemental window.
/// </summary>
public readonly record struct Window : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        ApplicationServiceInterop.FreeWindow(this);
    }
}
