namespace Elemental;

public readonly record struct NativeApplicationStatus
{
    private readonly uint _status;

    public NativeApplicationStatus()
    {
        _status = 0;
    }

    public bool IsActive
    {
        get
        {
            return (_status & (uint)NativeApplicationStatusFlags.Active) != 0;
        }
    }
}

[Flags]
public enum NativeApplicationStatusFlags : uint
{
    None = 0,
    Active = 1
}