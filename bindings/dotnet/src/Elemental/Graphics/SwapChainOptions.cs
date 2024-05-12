namespace Elemental.Graphics;

/// <summary>
/// Options for configuring a swap chain.
/// </summary>
public ref struct SwapChainOptions
{
    /// <summary>
    /// Custom payload for swap chain updates.
    /// </summary>
    public in void UpdatePayload { get; set; }

    /// <summary>
    /// Format of the swap chain.
    /// </summary>
    public SwapChainFormat Format { get; set; }

    /// <summary>
    /// Desired maximum number of frames in flight.
    /// </summary>
    public uint FrameLatency { get; set; }

    /// <summary>
    /// Target frames per second.
    /// </summary>
    public uint TargetFPS { get; set; }
}
