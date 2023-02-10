namespace Elemental.Graphics;

/// <summary>
/// Type of a <see cref="CommandQueue" />.
/// </summary>
public enum CommandType
{
    /// <summary>
    /// Graphics command type. This type of command can only be run by Graphics command queues.
    /// </summary>
    Graphics = 0,

    /// <summary>
    /// Compute command type. This type of command can be run by Graphics and Compute command queues.
    /// </summary>
    Compute = 1,
    
    /// <summary>
    /// Copy command type. This type of command can be run by all command queues.
    /// </summary>
    Copy = 2
}