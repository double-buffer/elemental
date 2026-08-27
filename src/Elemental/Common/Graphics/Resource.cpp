#include "Resource.h"
#include "GraphicsCommon.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

#ifdef _WIN32
#include "Microsoft/Graphics/DirectX12ResourceBarrier.h"
#include "Graphics/Vulkan/VulkanResourceBarrier.h"
#elif defined(__APPLE__)
#include "Apple/Graphics/MetalResourceBarrier.h"
#elif defined(__linux__)
#include "Graphics/Vulkan/VulkanResourceBarrier.h"
#endif

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

ElemAPI void ElemUploadGraphicsBufferData(ElemGraphicsResource buffer, uint32_t offset, ElemDataSpan data)
{
    DispatchGraphicsFunction(UploadGraphicsBufferData, buffer, offset, data);
}

ElemAPI ElemDataSpan ElemDownloadGraphicsBufferData(ElemGraphicsResource buffer, const ElemDownloadGraphicsBufferDataOptions* options)
{
    DispatchReturnGraphicsFunction(DownloadGraphicsBufferData, buffer, options);
}

ElemAPI void ElemCopyDataToGraphicsResource(ElemCommandList commandList, const ElemCopyDataToGraphicsResourceParameters* parameters)
{
    DispatchGraphicsFunction(CopyDataToGraphicsResource, commandList, parameters);
}

ElemAPI ElemGraphicsResourceDescriptor ElemCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options)
{
    auto resourceInfo = ElemGetGraphicsResourceInfo(resource);

    if (resourceInfo.Type == ElemGraphicsResourceType_Buffer && (resourceInfo.Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure))
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Raytracing acceleration structure storage buffers cannot have graphics resource descriptors.");
        return -1;
    }

    if (resourceInfo.Type == ElemGraphicsResourceType_RaytracingAccelerationStructure && usage == ElemGraphicsResourceDescriptorUsage_Write)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Raytracing acceleration structures only support read graphics resource descriptors.");
        return -1;
    }

    DispatchReturnGraphicsFunction(CreateGraphicsResourceDescriptor, resource, usage, options);
}

ElemAPI ElemGraphicsResourceDescriptorInfo ElemGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
    DispatchReturnGraphicsFunction(GetGraphicsResourceDescriptorInfo, descriptor);
}

ElemAPI void ElemFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options)
{
    if (descriptor == -1)
    {
        return;
    }

    DispatchGraphicsFunction(FreeGraphicsResourceDescriptor, descriptor, options);
}

ElemAPI void ElemGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    DispatchGraphicsFunction(GraphicsResourceBarrier, commandList, descriptor, options);
}

ElemAPI void ElemProcessGraphicsResourceDeleteQueue(ElemGraphicsDevice graphicsDevice)
{
    DispatchGraphicsFunction(ProcessGraphicsResourceDeleteQueue, graphicsDevice);
}

ElemAPI ElemGraphicsSampler ElemCreateGraphicsSampler(ElemGraphicsDevice graphicsDevice, const ElemGraphicsSamplerInfo* samplerInfo)
{
    DispatchReturnGraphicsFunction(CreateGraphicsSampler, graphicsDevice, samplerInfo);
}

ElemAPI ElemGraphicsSamplerInfo ElemGetGraphicsSamplerInfo(ElemGraphicsSampler sampler)
{
    DispatchReturnGraphicsFunction(GetGraphicsSamplerInfo, sampler);
}

ElemAPI void ElemFreeGraphicsSampler(ElemGraphicsSampler sampler, const ElemFreeGraphicsSamplerOptions* options)
{
    DispatchGraphicsFunction(FreeGraphicsSampler, sampler, options);
}

ElemAPI ElemRaytracingAllocationInfo ElemGetRaytracingBlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingBlasParameters* parameters)
{
    DispatchReturnGraphicsFunction(GetRaytracingBlasAllocationInfo, graphicsDevice, parameters);
}

ElemAPI ElemRaytracingAllocationInfo ElemGetRaytracingTlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingTlasParameters* parameters)
{
    DispatchReturnGraphicsFunction(GetRaytracingTlasAllocationInfo, graphicsDevice, parameters);
}

ElemAPI ElemGraphicsResourceAllocationInfo ElemGetRaytracingTlasInstanceAllocationInfo(ElemGraphicsDevice graphicsDevice, uint32_t instanceCount)
{
    DispatchReturnGraphicsFunction(GetRaytracingTlasInstanceAllocationInfo, graphicsDevice, instanceCount);
}

ElemAPI ElemDataSpan ElemEncodeRaytracingTlasInstances(ElemRaytracingTlasInstanceSpan instances)
{
    DispatchReturnGraphicsFunction(EncodeRaytracingTlasInstances, instances);
}

ElemAPI ElemGraphicsResource ElemCreateRaytracingAccelerationStructureResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResource storageBuffer, const ElemRaytracingAccelerationStructureOptions* options)
{
    DispatchReturnGraphicsFunction(CreateRaytracingAccelerationStructureResource, graphicsDevice, storageBuffer, options);
}

ElemAPI void ElemBuildRaytracingBlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingBlasParameters* parameters, const ElemRaytracingBuildOptions* options)
{
    DispatchGraphicsFunction(GraphicsResourceBarrierResource, commandList, accelerationStructure, ElemGraphicsResourceBarrierAccessType_Write);
    DispatchGraphicsFunction(BuildRaytracingBlas, commandList, accelerationStructure, scratchBuffer, parameters, options);
}

ElemAPI void ElemBuildRaytracingTlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingTlasParameters* parameters, const ElemRaytracingBuildOptions* options)
{
    DispatchGraphicsFunction(GraphicsResourceBarrierResource, commandList, accelerationStructure, ElemGraphicsResourceBarrierAccessType_Write);
    DispatchGraphicsFunction(BuildRaytracingTlas, commandList, accelerationStructure, scratchBuffer, parameters, options);
}