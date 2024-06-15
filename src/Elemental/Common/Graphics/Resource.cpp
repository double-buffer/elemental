#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI ElemGraphicsHeap ElemCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    DispatchReturnGraphicsFunction(CreateGraphicsHeap, graphicsDevice, sizeInBytes, options);
}

ElemAPI void ElemFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
    DispatchGraphicsFunction(FreeGraphicsHeap, graphicsHeap);
}

ElemAPI ElemGraphicsResource ElemCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo)
{
    DispatchReturnGraphicsFunction(CreateGraphicsResource, graphicsHeap, graphicsHeapOffset, resourceInfo);
}

ElemAPI void ElemFreeGraphicsResource(ElemGraphicsResource resource)
{
    DispatchGraphicsFunction(FreeGraphicsResource, resource);
}

ElemAPI ElemGraphicsResourceDescriptor ElemCreateGraphicsResourceDescriptor(const ElemGraphicsResourceDescriptorInfo* descriptorInfo)
{
    DispatchReturnGraphicsFunction(CreateGraphicsResourceDescriptor, descriptorInfo);
}

ElemAPI void ElemUpdateGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceDescriptorInfo* descriptorInfo)
{
    DispatchGraphicsFunction(UpdateGraphicsResourceDescriptor, descriptor, descriptorInfo);
}

ElemAPI void ElemFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor)
{
    DispatchGraphicsFunction(FreeGraphicsResourceDescriptor, descriptor);
}

ElemAPI void ElemGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor sourceDescriptor, ElemGraphicsResourceDescriptor destinationDescriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    DispatchGraphicsFunction(GraphicsResourceBarrier, commandList, sourceDescriptor, destinationDescriptor, options);
}
