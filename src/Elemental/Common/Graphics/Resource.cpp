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

ElemAPI ElemShaderDescriptor ElemCreateTextureShaderDescriptor(ElemGraphicsResource texture, const ElemTextureShaderDescriptorOptions* options)
{
    DispatchReturnGraphicsFunction(CreateTextureShaderDescriptor, texture, options);
}

ElemAPI void ElemFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor)
{
    DispatchGraphicsFunction(FreeShaderDescriptor, shaderDescriptor);
}
