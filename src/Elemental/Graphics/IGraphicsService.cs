namespace Elemental.Graphics;

/// <summary>
/// Manages low level GPU graphics resources and commands.
/// </summary>
[PlatformService] 
public interface IGraphicsService
{
    /// <summary>
    /// Creates a new <see cref="GraphicsDevice" />.
    /// </summary>
    /// <returns>Graphics device handle.</returns>
    GraphicsDevice CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics = GraphicsDiagnostics.None);

    /// <summary>
    /// Gets informations about the specified <see cref="GraphicsDevice" />.
    /// </summary>
    /// <param name="graphicsDevice">Graphics device handle.</param>
    /// <returns>Informations about the graphics device.</returns>     
    GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);
}