namespace Elemental;

/// <summary>
/// Represents the status of an application.
/// </summary>
public record struct ApplicationStatus
{
    /// <summary>
    /// Status code.
    /// </summary>
    public uint Status { get; set; }
}
