namespace Elemental.Graphics;

/// <summary>
/// Handle that represents a pipeline state.
/// </summary>
public readonly record struct PipelineState : IDisposable
{
    private UInt64 Value { get; }

    ///<summary>
    /// Disposes the handler.
    ///</summary>
    public void Dispose()
    {
        GraphicsServiceInterop.FreePipelineState(this);
    }
}
