namespace Elemental.Graphics;

public ref struct ExecuteCommandListOptions
{
    public bool InsertFence { get; set; }

    public bool FenceAwaitableOnCpu { get; set; }

    public FenceSpan FencesToWait { get; set; }
}
