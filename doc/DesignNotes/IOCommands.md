# Barriers

## Decisions to make

- [x] Do we use the existing CommandQueue/CommandList system?
- [ ] It would be nice to declare just heaps that are always GPU. 
      For Upload we use copy functions. That handle the copy operation.
      For readback we would have only a Readfunction that copy to CPU memory. (Provide a span)
      Only problem is vulkan that disallow the creation of VkImages to a ReBar Heap. :(

## Examples

1. Solution 1: Separate System

```c
    ElemIOCommandQueue ioCommandQueue = ElemCreateIOCommandQueue(NULL);
    
    ElemEnqueueIOCommand(&(ElemIOCommandParameters)
    {
        .DestinationResource = buffer,
        .DestinationOffset = 0,
        .SourceType = ElemIOCommandSourceType_File,
        .SourceFilename = "Test.mesh",
        .SourceFileOffset = 500,
        .SourceSizeInBytes = 233000,
    });

    ElemFence fence = ElemSubmitIOCommandQueue();
```

2. Solution 2: Reuse current CommandList system

```c
    // This should work with DirectStorage when using IO of with normal queues when using otherwise
    // TODO: IO or Transfer or Copy?
    ElemCommandQueue ioCommandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_IO, NULL);
    ElemCommandList ioCommandList = ElemGetCommandList(ioCommandQueue, NULL);

    // TODO: Simplify?
    ElemCopyDataToGraphicsResource(ioCommandList, &(ElemIOCommandParameters)
    {
        .DestinationResource = buffer,
        .DestinationOffset = 0,
        .SourceType = ElemIOCommandSourceType_File,
        .SourceFilename = "Test.mesh",
        .SourceFileOffset = 500,
        .SourceSizeInBytes = 233000,
    });

    // Direct upload with re-bar (no commandlist needed)
    ElemDirectCopyDataToGraphicsBuffer();

    ElemCommitCommandList(ioCommandList);
    ElemExecuteCommandList(ioCommandQueue, ioCommandList, NULL);

```
