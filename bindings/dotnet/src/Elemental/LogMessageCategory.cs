namespace Elemental;

/// <summary>
/// Enumerates the categories of log messages.
/// </summary>
public enum LogMessageCategory
{
    /// <summary>
    /// Memory related messages.
    /// </summary>
    Assert = 0,

    /// <summary>
    /// Memory related messages.
    /// </summary>
    Memory = 1,

    /// <summary>
    /// Native application messages.
    /// </summary>
    NativeApplication = 2,

    /// <summary>
    /// Graphics system messages.
    /// </summary>
    Graphics = 3,

    /// <summary>
    /// Input system messages.
    /// </summary>
    Inputs = 4
}
