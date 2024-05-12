namespace Elemental.Graphics;

/// <summary>
/// Enumerates types of command queues.
/// </summary>
public enum CommandQueueType
{
    /// <summary>
    /// Command queue for graphics operations.
    /// </summary>
    Graphics = 0,

    /// <summary>
    /// Command queue for compute operations.
    /// </summary>
    Compute = 1
}
