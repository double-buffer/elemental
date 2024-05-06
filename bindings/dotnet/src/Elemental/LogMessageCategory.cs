namespace Elemental;

/// <summary>
/// Categorizes log messages by their related system components.
/// </summary>
public enum LogMessageCategory
{
    /// <summary>
    /// Assertions and checks.
    /// </summary>
    Assert = 0,

    /// <summary>
    /// Memory allocation and management.
    /// </summary>
    Memory = 1,

    /// <summary>
    /// General application behavior.
    /// </summary>
    Application = 2,

    /// <summary>
    /// Graphics system-related messages.
    /// </summary>
    Graphics = 3,

    /// <summary>
    /// Input system-related messages.
    /// </summary>
    Inputs = 4
}
