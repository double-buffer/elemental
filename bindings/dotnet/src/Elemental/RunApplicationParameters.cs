namespace Elemental;

/// <summary>
/// Holds parameters for running an application, including initialization and cleanup routines.
/// </summary>
public ref struct RunApplicationParameters<T> 
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
    public ref T Payload;
}

internal unsafe struct RunApplicationParametersUnsafe
{
    public byte* ApplicationName { get; set; }

    public void* InitHandler { get; set; }

    public void* FreeHandler { get; set; }

    public void* Payload { get; set; }
}

