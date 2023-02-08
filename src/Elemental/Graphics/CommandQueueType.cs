namespace Elemental.Graphics;

/// <summary>
/// Type of a <see cref="CommandQueue" />.
/// </summary>
public enum CommandQueueType
{
    /// <summary>
    /// Render command queue. This queue can be used to do all GPU operations.
    /// </summary>
    Render = 0,

    /// <summary>
    /// Compute command queue. This queue can be used to do compute and copy operations.
    /// </summary>
    Compute = 1,
    
    /// <summary>
    /// Copy command queue. This queue can only be used to do copy operations.
    /// </summary>
    Copy = 2
}