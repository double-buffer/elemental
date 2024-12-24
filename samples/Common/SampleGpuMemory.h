#pragma once

#include "Elemental.h"
#include "SampleUtils.h"

typedef struct
{
    ElemGraphicsDevice GraphicsDevice;
    ElemGraphicsHeap GraphicsHeap;
    uint32_t CurrentHeapOffset;
} SampleGpuMemory;

typedef struct
{
    ElemGraphicsResource Buffer;
    ElemGraphicsResourceDescriptor ReadDescriptor;
} SampleGpuBuffer;

SampleGpuMemory SampleCreateGpuMemory(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes)
{
    // TODO: For now we need to put the heap as GpuUpload but it should be Gpu when we use IOQueues
    // TODO: Having GPU Upload is still annoying 😞
    ElemGraphicsHeap graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, sizeInBytes, &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_GpuUpload });

    return (SampleGpuMemory)
    {
        .GraphicsDevice = graphicsDevice,
        .GraphicsHeap = graphicsHeap,
        .CurrentHeapOffset = 0u
    };
}

void SampleFreeGpuMemory(SampleGpuMemory* gpuMemory)
{
    ElemFreeGraphicsHeap(gpuMemory->GraphicsHeap);
    gpuMemory->GraphicsHeap = ELEM_HANDLE_NULL;
}

// TODO: To remove when IOQueues
SampleGpuBuffer SampleCreateGpuBufferAndUploadData(SampleGpuMemory* gpuMemory, const void* dataPointer, uint32_t sizeInBytes, const char* debugName)
{
    // TODO: Alignment should be used with the offset before adding the size of the resource!
    ElemGraphicsResourceInfo bufferDescription = ElemCreateGraphicsBufferResourceInfo(gpuMemory->GraphicsDevice, sizeInBytes, ElemGraphicsResourceUsage_Read, &(ElemGraphicsResourceInfoOptions) { .DebugName = debugName });

    gpuMemory->CurrentHeapOffset = SampleAlignValue(gpuMemory->CurrentHeapOffset, bufferDescription.Alignment);
    ElemGraphicsResource buffer = ElemCreateGraphicsResource(gpuMemory->GraphicsHeap, gpuMemory->CurrentHeapOffset, &bufferDescription);
    gpuMemory->CurrentHeapOffset += bufferDescription.SizeInBytes;

    ElemGraphicsResourceDescriptor readDescriptor = ElemCreateGraphicsResourceDescriptor(buffer, ElemGraphicsResourceDescriptorUsage_Read, NULL);

    ElemDataSpan vertexBufferPointer = ElemGetGraphicsResourceDataSpan(buffer);
    memcpy(vertexBufferPointer.Items, dataPointer, sizeInBytes);

    return (SampleGpuBuffer)
    {
        .Buffer = buffer,
        .ReadDescriptor = readDescriptor
    };
}

void SampleFreeGpuBuffer(SampleGpuBuffer* gpuBuffer)
{
    ElemFreeGraphicsResourceDescriptor(gpuBuffer->ReadDescriptor, NULL);
    gpuBuffer->ReadDescriptor = ELEM_HANDLE_NULL;

    ElemFreeGraphicsResource(gpuBuffer->Buffer, NULL);
    gpuBuffer->Buffer = ELEM_HANDLE_NULL;
}
