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

typedef struct
{
    ElemGraphicsResource Texture;
    ElemGraphicsResourceDescriptor ReadDescriptor;
} SampleGpuTexture;

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

SampleGpuBuffer SampleCreateGpuBuffer(SampleGpuMemory* gpuMemory, uint32_t sizeInBytes, const char* debugName)
{
    // TODO: Alignment should be used with the offset before adding the size of the resource!
    ElemGraphicsResourceInfo bufferDescription = ElemCreateGraphicsBufferResourceInfo(gpuMemory->GraphicsDevice, sizeInBytes, ElemGraphicsResourceUsage_Read, &(ElemGraphicsResourceInfoOptions) { .DebugName = debugName });

    gpuMemory->CurrentHeapOffset = SampleAlignValue(gpuMemory->CurrentHeapOffset, bufferDescription.Alignment);
    ElemGraphicsResource buffer = ElemCreateGraphicsResource(gpuMemory->GraphicsHeap, gpuMemory->CurrentHeapOffset, &bufferDescription);
    gpuMemory->CurrentHeapOffset += bufferDescription.SizeInBytes;

    ElemGraphicsResourceDescriptor readDescriptor = ElemCreateGraphicsResourceDescriptor(buffer, ElemGraphicsResourceDescriptorUsage_Read, NULL);

    return (SampleGpuBuffer)
    {
        .Buffer = buffer,
        .ReadDescriptor = readDescriptor
    };
}

// TODO: To remove when IOQueues
SampleGpuBuffer SampleCreateGpuBufferAndUploadData(SampleGpuMemory* gpuMemory, const void* dataPointer, uint32_t sizeInBytes, const char* debugName)
{
    SampleGpuBuffer result = SampleCreateGpuBuffer(gpuMemory, sizeInBytes, debugName);
    ElemUploadGraphicsBufferData(result.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)dataPointer, .Length = sizeInBytes });

    return (SampleGpuBuffer)
    {
        .Buffer = result.Buffer,
        .ReadDescriptor = result.ReadDescriptor
    };
}

void SampleFreeGpuBuffer(SampleGpuBuffer* gpuBuffer)
{
    ElemFreeGraphicsResourceDescriptor(gpuBuffer->ReadDescriptor, NULL);
    gpuBuffer->ReadDescriptor = ELEM_HANDLE_NULL;

    ElemFreeGraphicsResource(gpuBuffer->Buffer, NULL);
    gpuBuffer->Buffer = ELEM_HANDLE_NULL;
}

SampleGpuTexture SampleCreateGpuTexture(SampleGpuMemory* gpuMemory, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, const char* debugName)
{
    ElemGraphicsResourceInfo textureDescription = ElemCreateTexture2DResourceInfo(gpuMemory->GraphicsDevice, width, height, mipLevels, format, ElemGraphicsResourceUsage_Read, &(ElemGraphicsResourceInfoOptions) { .DebugName = debugName });

    gpuMemory->CurrentHeapOffset = SampleAlignValue(gpuMemory->CurrentHeapOffset, textureDescription.Alignment);
    ElemGraphicsResource texture = ElemCreateGraphicsResource(gpuMemory->GraphicsHeap, gpuMemory->CurrentHeapOffset, &textureDescription);
    gpuMemory->CurrentHeapOffset += textureDescription.SizeInBytes;

    ElemGraphicsResourceDescriptor readDescriptor = ElemCreateGraphicsResourceDescriptor(texture, ElemGraphicsResourceDescriptorUsage_Read, NULL);

    return (SampleGpuTexture)
    {
        .Texture = texture,
        .ReadDescriptor = readDescriptor
    };
}

void SampleFreeGpuTexture(SampleGpuTexture* gpuTexture)
{
    assert(gpuTexture);

    ElemFreeGraphicsResourceDescriptor(gpuTexture->ReadDescriptor, NULL);
    gpuTexture->ReadDescriptor = ELEM_HANDLE_NULL;

    ElemFreeGraphicsResource(gpuTexture->Texture, NULL);
    gpuTexture->Texture = ELEM_HANDLE_NULL;
}
