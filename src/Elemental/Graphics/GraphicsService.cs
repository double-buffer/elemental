namespace Elemental.Graphics;

public sealed partial class GraphicsService
{
    /// <summary>
    /// Executes a <see cref="CommandList" /> on the specified <see cref="CommandQueue" />.
    /// </summary>
    /// <param name="commandQueue">CommandQueue to use.</param>
    /// <param name="commandList">Command list to execute.</param>
    /// <returns>Fence object that will be signaled when the execution is finished on the GPU.</returns>
    public Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList)
    {
        var commandLists = MemoryMarshal.CreateReadOnlySpan(ref commandList, 1);
        return ExecuteCommandLists(commandQueue, commandLists, Array.Empty<Fence>());
    }
}