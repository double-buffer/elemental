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

ElemAPI ElemTexture ElemCreateTexture(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemTextureParameters* parameters)
{
    DispatchReturnGraphicsFunction(CreateTexture, graphicsHeap, graphicsHeapOffset, parameters);
}

ElemAPI void ElemFreeTexture(ElemTexture texture)
{
    DispatchGraphicsFunction(FreeTexture, texture);
}

ElemAPI ElemShaderDescriptor ElemCreateTextureShaderDescriptor(ElemTexture texture, const ElemTextureShaderDescriptorOptions* options)
{
    DispatchReturnGraphicsFunction(CreateTextureShaderDescriptor, texture, options);
}

ElemAPI void ElemFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor)
{
    DispatchGraphicsFunction(FreeShaderDescriptor, shaderDescriptor);
}
