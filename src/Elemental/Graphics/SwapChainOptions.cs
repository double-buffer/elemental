namespace Elemental.Graphics;

/// <summary>
/// Describes a <see cref="SwapChain" /> to create.
/// </summary>
public readonly record struct SwapChainOptions
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public SwapChainOptions()
    {
        Width = 0;
        Height = 0;
        Format = SwapChainFormat.Default;
        MaximumFrameLatency = 2;
    }

    /// <summary>
    /// Width in pixels of the <see cref="SwapChain" />.
    /// </summary>
    /// <value>Width in pixels.</value>
    public int Width { get; init; }
    
    /// <summary>
    /// Height in pixels of the <see cref="SwapChain" />.
    /// </summary>
    /// <value>Height in pixels.</value>
    public int Height { get; init; }

    /// <summary>
    /// Format of the <see cref="SwapChain" />.
    /// </summary>
    /// <value>Format of the swap chain.</value>
    public SwapChainFormat Format { get; init; }

    /// <summary>
    /// Maximum frame latency of the <see cref="SwapChain" />.
    /// This value represents the maximum number of back buffers that can be
    /// submitted at the same time. Maximum value is 3 and minimum is 1.
    /// Default value is 2.
    /// </summary>
    /// <value>Maximum frame latency.</value>
    /// <remarks>
    /// Setting this value to 1 can reduce latency but reduce the ability for the
    /// CPU and GPU to work in parallel. 
    /// </remarks>
    public int MaximumFrameLatency { get; init; }

    // TODO: Provide options for VRR
}