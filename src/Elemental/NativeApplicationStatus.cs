namespace Elemental;

public readonly record struct NativeApplicationStatus
{
    private readonly uint _status;

    public NativeApplicationStatus()
    {
        _status = 0;
    }

    public bool IsActive => CheckStatus(NativeApplicationStatusFlags.Active);
    public bool IsClosing => CheckStatus(NativeApplicationStatusFlags.Closing);

    private bool CheckStatus(NativeApplicationStatusFlags flag)
    {
        return (_status & (uint)flag) != 0;
    }
}

[Flags]
internal enum NativeApplicationStatusFlags : uint
{
    None = 0,
    Active = 1,
    Closing = 2
}