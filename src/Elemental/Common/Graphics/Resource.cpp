#include "Resource.h"
#include "GraphicsCommon.h"

bool CheckDepthStencilFormat(ElemGraphicsFormat format)
{
    if (format == ElemGraphicsFormat_D32_FLOAT)
    {
        return true;
    }

    return false;
}

ElemAPI ElemGraphicsHeap ElemCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    DispatchReturnGraphicsFunction(CreateGraphicsHeap, graphicsDevice, sizeInBytes, options);
}

ElemAPI void ElemFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
    DispatchGraphicsFunction(FreeGraphicsHeap, graphicsHeap);
}

ElemAPI ElemGraphicsResourceInfo ElemCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    DispatchReturnGraphicsFunction(CreateGraphicsBufferResourceInfo, graphicsDevice, sizeInBytes, usage, options);
}

ElemAPI ElemGraphicsResourceInfo ElemCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    DispatchReturnGraphicsFunction(CreateTexture2DResourceInfo, graphicsDevice, width, height, mipLevels, format, usage, options);
}

ElemAPI ElemGraphicsResource ElemCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo)
{
    DispatchReturnGraphicsFunction(CreateGraphicsResource, graphicsHeap, graphicsHeapOffset, resourceInfo);
}

ElemAPI void ElemFreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options)
{
    DispatchGraphicsFunction(FreeGraphicsResource, resource, options);
}

ElemAPI ElemGraphicsResourceInfo ElemGetGraphicsResourceInfo(ElemGraphicsResource resource)
{
    DispatchReturnGraphicsFunction(GetGraphicsResourceInfo, resource);
}

ElemAPI void ElemUploadGraphicsBufferData(ElemGraphicsResource resource, uint32_t offset, ElemDataSpan data)
{
    DispatchGraphicsFunction(UploadGraphicsBufferData, resource, offset, data);
}

ElemAPI ElemDataSpan ElemDownloadGraphicsBufferData(ElemGraphicsResource resource, const ElemDownloadGraphicsBufferDataOptions* options)
{
    DispatchReturnGraphicsFunction(DownloadGraphicsBufferData, resource, options);
}

ElemAPI ElemGraphicsResourceDescriptor ElemCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options)
{
    DispatchReturnGraphicsFunction(CreateGraphicsResourceDescriptor, resource, usage, options);
}

ElemAPI ElemGraphicsResourceDescriptorInfo ElemGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
    DispatchReturnGraphicsFunction(GetGraphicsResourceDescriptorInfo, descriptor);
}

ElemAPI void ElemFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options)
{
    DispatchGraphicsFunction(FreeGraphicsResourceDescriptor, descriptor, options);
}

ElemAPI void ElemGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    DispatchGraphicsFunction(GraphicsResourceBarrier, commandList, descriptor, options);
}

ElemAPI void ElemProcessGraphicsResourceDeleteQueue()
{
    DispatchGraphicsFunction(ProcessGraphicsResourceDeleteQueue);
}
