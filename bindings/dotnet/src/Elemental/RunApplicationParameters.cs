namespace Elemental;

/// <summary>
/// Holds parameters for running an application, including initialization and cleanup routines.
/// </summary>
public ref struct RunApplicationParameters<T> where T: unmanaged
{
    /// <summary>
    /// Name of the application.
    /// </summary>
    public ReadOnlySpan<byte> ApplicationName { get; set; }

    /// <summary>
    /// Function called at application startup.
    /// </summary>
    public ApplicationHandler<T> InitHandler { get; set; }

    /// <summary>
    /// Function called at application termination.
    /// </summary>
    public ApplicationHandler<T> FreeHandler { get; set; }

    /// <summary>
    /// Custom user data passed to handler functions.
    /// </summary>
    public T Payload { get; set; }
}

internal unsafe struct RunApplicationParametersUnsafe
{
    public byte* ApplicationName { get; set; }

    public nint InitHandler { get; set; }

    public nint FreeHandler { get; set; }

    public nint Payload;
}

