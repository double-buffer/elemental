namespace Elemental;

/// <summary>
/// Contains information about the system, useful for tailoring application behavior.
/// </summary>
public record struct SystemInfo
{
    /// <summary>
    /// Operating system platform.
    /// </summary>
    public Platform Platform { get; set; }

    /// <summary>
    /// Installation path of the application.
    /// </summary>
    public in char ApplicationPath { get; set; }

    /// <summary>
    /// Whether the application supports multiple windows.
    /// </summary>
    public bool SupportMultiWindows { get; set; }
}
