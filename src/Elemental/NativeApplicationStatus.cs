namespace Elemental;

/// <summary>
/// Status of a <see cref="NativeApplication" />.
/// </summary>
public readonly record struct NativeApplicationStatus
{
    private readonly uint _status = 0;

    /// <summary>
    /// Default constructor.
    /// </summary>
    public NativeApplicationStatus()
    {
    }

    /// <summary>
    /// Gets the a boolean indicating if the application is active.
    /// </summary>
    /// <returns>True if the application is active; False otherwise.</returns>
    public bool IsActive => CheckStatus(NativeApplicationStatusFlags.Active);
    
    /// <summary>
    /// Gets the a boolean indicating if the application is closing.
    /// </summary>
    /// <returns>True if the application is closing; False otherwise.</returns>
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