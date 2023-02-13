namespace Elemental.Graphics;

/// <summary>
/// Type of a <see cref="CommandQueue" />.
/// </summary>
public enum CommandQueueType
{
    /// <summary>
    /// Graphics command queue type. This type of command queue can process all GPU commands.
    /// </summary>
    Graphics = 0,

    /// <summary>
    /// Compute command queue type. This type of command queue can be run only copy and compute commands.
    /// </summary>
    Compute = 1,
    
    /// <summary>
    /// Copy command queue type. This type of command queue can run only copy commands.
    /// </summary>
    Copy = 2
}