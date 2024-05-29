namespace Elemental.Graphics;

/// <summary>
/// Options for creating a command queue.
/// </summary>
public ref struct CommandQueueOptions
{
    /// <summary>
    /// Optional debug name for the command queue.
    /// </summary>
    public in char DebugName { get; set; }
}
