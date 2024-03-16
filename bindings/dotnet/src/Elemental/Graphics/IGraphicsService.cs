namespace Elemental.Graphics;

/// <summary>
/// Defines an interface for Graphics services.
/// </summary>
public interface IGraphicsService
{
    void SetGraphicsOptions(in GraphicsOptions options = default);

    GraphicsDeviceInfoList GetAvailableGraphicsDevices();

    GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options = default);

    void FreeGraphicsDevice(GraphicsDevice graphicsDevice);

    GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);

    GraphicsCommandQueue CreateGraphicsCommandQueue(GraphicsDevice graphicsDevice, GraphicsCommandQueueType type, in GraphicsCommandQueueOptions options = default);
}
