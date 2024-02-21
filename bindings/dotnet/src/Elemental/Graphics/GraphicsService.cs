namespace Elemental;

/// <inheritdoc />
public class GraphicsService : IGraphicsService
{
    /// <inheritdoc />
    public GraphicsDevice CreateGraphicsDevice()
    {
        return GraphicsServiceInterop.CreateGraphicsDevice();
    }

    /// <inheritdoc />
    public void FreeGraphicsDevice(GraphicsDevice graphicsDevice)
    {
        GraphicsServiceInterop.FreeGraphicsDevice(graphicsDevice);
    }

}
