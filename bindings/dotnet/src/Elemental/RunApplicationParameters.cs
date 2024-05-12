namespace Elemental;

/// <summary>
/// Holds parameters for running an application, including initialization and cleanup routines.
/// </summary>
public ref struct RunApplicationParameters
{
    /// <summary>
    /// Name of the application.
    /// </summary>
    public ReadOnlySpan<byte> ApplicationName { get; set; }

    /// <summary>
    /// Function called at application startup.
    /// </summary>
    public ApplicationHandler InitHandler { get; set; }

    /// <summary>
    /// Function called at application termination.
    /// </summary>
    public ApplicationHandler FreeHandler { get; set; }

    /// <summary>
    /// Custom user data passed to handler functions.
    /// </summary>
    public in void Payload { get; set; }
}

internal unsafe struct RunApplicationParametersUnsafe
{
    public byte* ApplicationName { get; set; }

    public ApplicationHandler InitHandler { get; set; }

    public ApplicationHandler FreeHandler { get; set; }

    public in void Payload { get; set; }
}

