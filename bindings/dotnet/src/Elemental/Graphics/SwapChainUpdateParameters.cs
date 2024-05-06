namespace Elemental.Graphics;

/// <summary>
/// Parameters for updating a swap chain during rendering.
/// </summary>
public ref struct SwapChainUpdateParameters
{
    /// <summary>
    /// Information about the swap chain's configuration.
    /// </summary>
    public SwapChainInfo SwapChainInfo { get; set; }

    /// <summary>
    /// Back buffer texture for the swap chain.
    /// </summary>
    public Texture BackBufferTexture { get; set; }

    /// <summary>
    /// Time since the last frame was presented, in seconds.
    /// </summary>
    public double DeltaTimeInSeconds { get; set; }

    /// <summary>
    /// Timestamp for when the next frame is expected to be presented, in seconds.
    /// </summary>
    public double NextPresentTimeStampInSeconds { get; set; }
}
