namespace Elemental;

/// <summary>
/// Contains information about the system, useful for tailoring application behavior.
/// </summary>
public ref struct SystemInfo
{
    /// <summary>
    /// Operating system platform.
    /// </summary>
    public Platform Platform { get; set; }

    /// <summary>
    /// Installation path of the application.
    /// </summary>
    public ReadOnlySpan<byte> ApplicationPath { get; set; }

    /// <summary>
    /// Whether the application supports multiple windows.
    /// </summary>
    public bool SupportMultiWindows { get; set; }
}

internal unsafe struct SystemInfoUnsafe
{
    public Platform Platform { get; set; }

    public byte* ApplicationPath { get; set; }

    public bool SupportMultiWindows { get; set; }
}

