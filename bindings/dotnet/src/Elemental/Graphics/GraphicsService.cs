namespace Elemental.Graphics;

/// <inheritdoc />
public class GraphicsService : IGraphicsService
{
    /// <summary>
    /// Temporary Function
    /// </summary>
    public GraphicsDevice CreateGraphicsDevice()
    {
        return GraphicsServiceInterop.CreateGraphicsDevice();
    }

    /// <summary>
    /// Temporary Function
    /// </summary>
    public void FreeGraphicsDevice(GraphicsDevice graphicsDevice)
    {
        GraphicsServiceInterop.FreeGraphicsDevice(graphicsDevice);
    }

}
