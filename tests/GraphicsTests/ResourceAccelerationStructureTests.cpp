#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Check the validation of all parameters
// TODO: Check when we pass 0 to the buffers size (should be resource size then)
// TODO: Check when passing different flags that memory alloc info is different
// TODO: Build BLAS test that we pass an acceleration structure
// TODO: Check that the storage buffer passed to the create accel function was create with usage AccelerationStructure
// TODO: Test encode TLAS Instances function

UTEST(ResourceAccelerationStructure, GetRaytracingBlasAllocationInfo) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto vertexBuffer = TestCreateGpuBuffer(graphicsDevice, 1024);
    auto indexBuffer = TestCreateGpuBuffer(graphicsDevice, 256);
    auto vertexSizeInBytes = (uint32_t)sizeof(float) * 3u;

    ElemRaytracingBlasParameters parameters
    {
        .BuildFlags = (ElemRaytracingBuildFlags)(ElemRaytracingBuildFlags_PreferFastTrace | ElemRaytracingBuildFlags_AllowUpdate),
        .VertexFormat = ElemGraphicsFormat_R32G32B32_FLOAT,
        .VertexBuffer = vertexBuffer.Buffer,
        .VertexBufferOffset = 0,
        .VertexCount = 50,
        .VertexSizeInBytes = vertexSizeInBytes,
        .IndexFormat = ElemGraphicsFormat_R32_UINT,
        .IndexBuffer = indexBuffer.Buffer,
        .IndexBufferOffset = 0,
        .IndexCount = 20
    };

    // Act
    auto allocationInfo = ElemGetRaytracingBlasAllocationInfo(graphicsDevice, &parameters); 

    // Assert
    TestFreeGpuBuffer(vertexBuffer);
    TestFreeGpuBuffer(indexBuffer);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(allocationInfo.SizeInBytes, 0u, "SizeInBytes should not be equals to 0.");
}

UTEST(ResourceAccelerationStructure, CreateRaytracingAccelerationStructureResource) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto storageBuffer = TestCreateGpuBuffer(graphicsDevice, 1024, ElemGraphicsHeapType_Gpu, ElemGraphicsResourceUsage_RaytracingAccelerationStructure);

    // Act
    auto accelerationStructure = ElemCreateRaytracingAccelerationStructureResource(graphicsDevice, storageBuffer.Buffer, nullptr);

    // Assert
    TestFreeGpuBuffer(storageBuffer);
    ElemFreeGraphicsResource(accelerationStructure, nullptr);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(accelerationStructure, ELEM_HANDLE_NULL, "Handle should not be null.");
}

UTEST(ResourceAccelerationStructure, BuildRaytracingBlas) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto vertexBuffer = TestCreateGpuBuffer(graphicsDevice, 1024);
    auto indexBuffer = TestCreateGpuBuffer(graphicsDevice, 256);
    auto vertexSizeInBytes = (uint32_t)sizeof(float) * 3u;

    ElemRaytracingBlasParameters parameters
    {
        .BuildFlags = (ElemRaytracingBuildFlags)(ElemRaytracingBuildFlags_PreferFastTrace | ElemRaytracingBuildFlags_AllowUpdate),
        .VertexFormat = ElemGraphicsFormat_R32G32B32_FLOAT,
        .VertexBuffer = vertexBuffer.Buffer,
        .VertexBufferOffset = 0,
        .VertexCount = 50,
        .VertexSizeInBytes = vertexSizeInBytes,
        .IndexFormat = ElemGraphicsFormat_R32_UINT,
        .IndexBuffer = indexBuffer.Buffer,
        .IndexBufferOffset = 0,
        .IndexCount = 20
    };

    auto allocationInfo = ElemGetRaytracingBlasAllocationInfo(graphicsDevice, &parameters); 
    auto storageBuffer = TestCreateGpuBuffer(graphicsDevice, allocationInfo.SizeInBytes, ElemGraphicsHeapType_Gpu, ElemGraphicsResourceUsage_RaytracingAccelerationStructure);
    auto scratchBuffer = TestCreateGpuBuffer(graphicsDevice, allocationInfo.ScratchSizeInBytes);

    auto accelerationStructure = ElemCreateRaytracingAccelerationStructureResource(graphicsDevice, storageBuffer.Buffer, nullptr);

    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemBuildRaytracingBlas(commandList, accelerationStructure, scratchBuffer.Buffer, &parameters, nullptr);

    // Assert
    ElemCommitCommandList(commandList);

    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    TestFreeGpuBuffer(vertexBuffer);
    TestFreeGpuBuffer(indexBuffer);
    TestFreeGpuBuffer(storageBuffer);
    TestFreeGpuBuffer(scratchBuffer);
    ElemFreeGraphicsResource(accelerationStructure, nullptr);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(allocationInfo.SizeInBytes, 0u, "SizeInBytes should not be equals to 0.");
}
