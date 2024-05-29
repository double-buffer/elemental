namespace Elemental.Graphics;

/// <summary>
/// Options for executing a command list.
/// </summary>
public ref struct ExecuteCommandListOptions
{
    /// <summary>
    /// TODO: Do we need that? This was foreseen for metal because SharedEvent may be slower
///But it would be great to avoid this flag.
///If set to true, CPU can wait on the fence.
    /// </summary>
    public bool FenceAwaitableOnCpu { get; set; }

    /// <summary>
    /// Fences that the execution should wait on before starting.
    /// </summary>
    public ReadOnlySpan<Fence> FencesToWait { get; set; }
}

internal unsafe struct ExecuteCommandListOptionsUnsafe
{
    public bool FenceAwaitableOnCpu { get; set; }

    public Fence* FencesToWait { get; set; }
}

