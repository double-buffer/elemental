namespace Elemental.Graphics;

/// <summary>
/// Options for executing a command list.
/// </summary>
public ref struct ExecuteCommandListOptions
{
    /// <summary>
    /// If set to true, CPU can wait on the fence.
    /// </summary>
    public bool FenceAwaitableOnCpu { get; set; }

    /// <summary>
    /// Fences that the execution should wait on before starting.
    /// </summary>
    public ReadOnlyMemory<Fence> FencesToWait { get; set; }
}

internal unsafe struct ExecuteCommandListOptionsUnsafe
{
    public bool FenceAwaitableOnCpu { get; set; }

    public Fence* FencesToWait { get; set; }
}

