namespace Elemental;

/// <summary>
/// Handle that represents an elemental application.
/// </summary>
public readonly record struct ElementalApplication : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        ApplicationServiceInterop.FreeApplication(this);
    }
}
