namespace Elemental.Graphics;

public ref struct ExecuteCommandListOptions
{
    public bool InsertFence { get; set; }

    public bool FenceAwaitableOnCpu { get; set; }

    public ReadOnlyMemory<Fence> FencesToWait { get; set; }
}

internal unsafe struct ExecuteCommandListOptionsUnsafe
{
    public bool InsertFence { get; set; }

    public bool FenceAwaitableOnCpu { get; set; }

    public Fence* FencesToWait { get; set; }
}

