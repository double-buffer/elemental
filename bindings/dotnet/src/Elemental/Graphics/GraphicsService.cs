namespace Elemental.Graphics;

/// <inheritdoc />
public class GraphicsService : IGraphicsService
{
    public void SetGraphicsOptions(in GraphicsOptions options = default)
    {
        GraphicsServiceInterop.SetGraphicsOptions(options);
    }

    public GraphicsDeviceInfoList GetAvailableGraphicsDevices()
    {
        return GraphicsServiceInterop.GetAvailableGraphicsDevices();
    }

    public GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options = default)
    {
        return GraphicsServiceInterop.CreateGraphicsDevice(options);
    }

    public void FreeGraphicsDevice(GraphicsDevice graphicsDevice)
    {
        GraphicsServiceInterop.FreeGraphicsDevice(graphicsDevice);
    }

    public GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice)
    {
        return GraphicsServiceInterop.GetGraphicsDeviceInfo(graphicsDevice);
    }

    public unsafe GraphicsCommandQueue CreateGraphicsCommandQueue(GraphicsDevice graphicsDevice, GraphicsCommandQueueType type, in GraphicsCommandQueueOptions options = default)
    {
        fixed (byte* DebugNamePinned = options.DebugName)
        {
            var optionsUnsafe = new GraphicsCommandQueueOptionsUnsafe();
            optionsUnsafe.DebugName = DebugNamePinned;

            return GraphicsServiceInterop.CreateGraphicsCommandQueue(graphicsDevice, type, optionsUnsafe);
        }
    }

}
