namespace Elemental.Graphics;

/// <summary>
/// Options for creating a command list.
/// </summary>
public ref struct CommandListOptions
{
    /// <summary>
    /// Optional debug name for the command list.
    /// </summary>
    public in char DebugName { get; set; }
}
