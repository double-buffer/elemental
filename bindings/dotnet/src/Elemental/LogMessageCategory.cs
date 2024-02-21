namespace Elemental;

/// <summary>
/// Enumerates the categories of log messages.
/// </summary>
public enum LogMessageCategory
{
    /// <summary>
    /// Memory related messages.
    /// </summary>
    Memory = 0,

    /// <summary>
    /// Native application messages.
    /// </summary>
    NativeApplication = 1,

    /// <summary>
    /// Graphics system messages.
    /// </summary>
    Graphics = 2,

    /// <summary>
    /// Input system messages.
    /// </summary>
    Inputs = 3
}
