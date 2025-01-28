#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Check the validation of all parameters
// TODO: Check when we pass 0 to the buffers size (should be resource size then)
// TODO: Test encode TLAS Instances function

UTEST(Raytracing, GetRaytracingBlasAllocationInfo) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto vertexSizeInBytes = (uint32_t)sizeof(float) * 3u;

    ElemRaytracingBlasGeometry geometry =
    {
        .VertexFormat = ElemGraphicsFormat_R32G32B32_FLOAT,
        .VertexCount = 50,
        .VertexSizeInBytes = vertexSizeInBytes,
        .IndexFormat = ElemGraphicsFormat_R32_UINT,
        .IndexCount = 20
    };

    ElemRaytracingBlasParameters parameters
    {
        .BuildFlags = (ElemRaytracingBuildFlags)(ElemRaytracingBuildFlags_PreferFastTrace | ElemRaytracingBuildFlags_AllowUpdate),
        .GeometryList = { .Items = &geometry, .Length = 1 }
    };

    // Act
    auto allocationInfo = ElemGetRaytracingBlasAllocationInfo(graphicsDevice, &parameters); 

    // Assert
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(allocationInfo.SizeInBytes, 0u, "SizeInBytes should not be equals to 0.");
}

UTEST(Raytracing, GetRaytracingTlasAllocationInfo) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemRaytracingTlasParameters parameters
    {
        .BuildFlags = (ElemRaytracingBuildFlags)(ElemRaytracingBuildFlags_PreferFastTrace | ElemRaytracingBuildFlags_AllowUpdate),
        .InstanceCount = 24
    };

    // Act
    auto allocationInfo = ElemGetRaytracingTlasAllocationInfo(graphicsDevice, &parameters); 

    // Assert
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(allocationInfo.SizeInBytes, 0u, "SizeInBytes should not be equals to 0.");
}

UTEST(Raytracing, GetRaytracingTlasInstanceAllocationInfo) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    // Act
    auto allocationInfo = ElemGetRaytracingTlasInstanceAllocationInfo(graphicsDevice, 24); 

    // Assert
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(allocationInfo.SizeInBytes, 0u, "SizeInBytes should not be equals to 0.");
}

UTEST(Raytracing, CreateGraphicsBufferForAccelerationStructure_NotOnGpuHeap) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    // Act
    auto storageBuffer = TestCreateGpuBuffer(graphicsDevice, 1024, ElemGraphicsHeapType_GpuUpload, ElemGraphicsResourceUsage_RaytracingAccelerationStructure);

    // Assert
    TestFreeGpuBuffer(storageBuffer);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("GraphicsBuffer with usage RaytracingAccelerationStructure should be allocated on a Gpu Heap.");
    ASSERT_EQ_MSG(storageBuffer.Buffer, ELEM_HANDLE_NULL, "Handle should be null.");
}

UTEST(Raytracing, CreateRaytracingAccelerationStructureResource_StorageBufferWrongUsage) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto storageBuffer = TestCreateGpuBuffer(graphicsDevice, 1024, ElemGraphicsHeapType_Gpu, ElemGraphicsResourceUsage_Read);

    // Act
    auto accelerationStructure = ElemCreateRaytracingAccelerationStructureResource(graphicsDevice, storageBuffer.Buffer, nullptr);

    // Assert
    TestFreeGpuBuffer(storageBuffer);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("RaytracingAccelerationStructure need to have a storage buffer that was created with RaytracingAccelerationStructure usage.");
    ASSERT_EQ_MSG(accelerationStructure, ELEM_HANDLE_NULL, "Handle should be null.");
}

UTEST(Raytracing, CreateRaytracingAccelerationStructureResource) 
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

UTEST(Raytracing, BuildRaytracingBlas) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto vertexBuffer = TestCreateGpuBuffer(graphicsDevice, 1024);
    auto indexBuffer = TestCreateGpuBuffer(graphicsDevice, 256);
    auto vertexSizeInBytes = (uint32_t)sizeof(float) * 3u;

    ElemRaytracingBlasGeometry geometry =
    {
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

    ElemRaytracingBlasParameters parameters
    {
        .BuildFlags = (ElemRaytracingBuildFlags)(ElemRaytracingBuildFlags_PreferFastTrace | ElemRaytracingBuildFlags_AllowUpdate),
        .GeometryList = { .Items = &geometry, .Length = 1 }
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

UTEST(Raytracing, BuildRaytracingBlas_NotAccelerationStructureResource) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto vertexBuffer = TestCreateGpuBuffer(graphicsDevice, 1024);
    auto indexBuffer = TestCreateGpuBuffer(graphicsDevice, 256);
    auto vertexSizeInBytes = (uint32_t)sizeof(float) * 3u;

    ElemRaytracingBlasGeometry geometry =
    {
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

    ElemRaytracingBlasParameters parameters
    {
        .BuildFlags = (ElemRaytracingBuildFlags)(ElemRaytracingBuildFlags_PreferFastTrace | ElemRaytracingBuildFlags_AllowUpdate),
        .GeometryList = { .Items = &geometry, .Length = 1 }
    };

    auto storageBuffer = TestCreateGpuBuffer(graphicsDevice, 1024, ElemGraphicsHeapType_Gpu);
    auto scratchBuffer = TestCreateGpuBuffer(graphicsDevice, 1024);

    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemBuildRaytracingBlas(commandList, storageBuffer.Buffer, scratchBuffer.Buffer, &parameters, nullptr);

    // Assert
    ElemCommitCommandList(commandList);

    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    TestFreeGpuBuffer(vertexBuffer);
    TestFreeGpuBuffer(indexBuffer);
    TestFreeGpuBuffer(storageBuffer);
    TestFreeGpuBuffer(scratchBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("Acceleration structure is not an acceleration structure graphics resource.");
}

UTEST(Raytracing, ElemEncodeRaytracingTlasInstances) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto storageBuffer = TestCreateGpuBuffer(graphicsDevice, 1024, ElemGraphicsHeapType_Gpu, ElemGraphicsResourceUsage_RaytracingAccelerationStructure);
    auto blas = ElemCreateRaytracingAccelerationStructureResource(graphicsDevice, storageBuffer.Buffer, nullptr);

    auto allocationInfos = ElemGetRaytracingTlasInstanceAllocationInfo(graphicsDevice, 10);
    auto instancesStorageBuffer = TestCreateGpuBuffer(graphicsDevice, allocationInfos.SizeInBytes, ElemGraphicsHeapType_GpuUpload, ElemGraphicsResourceUsage_Read);
    ElemRaytracingTlasInstance tlasInstances[10];

    for (uint32_t i = 0; i < 10; i++)
    {
        tlasInstances[i] = (ElemRaytracingTlasInstance)
        {
            .InstanceId = i,
            .InstanceMask = 1,
            .BlasResource = blas
        };
    }
    
    // Act
    auto tlasInstanceData = ElemEncodeRaytracingTlasInstances((ElemRaytracingTlasInstanceSpan) { .Items = tlasInstances, .Length = 10 });

    // Assert
    TestFreeGpuBuffer(storageBuffer);
    TestFreeGpuBuffer(instancesStorageBuffer);
    ElemFreeGraphicsResource(blas, nullptr);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(tlasInstanceData.Length, allocationInfos.SizeInBytes, "Tlas Instance length should be equals to 10.");
}

UTEST(Raytracing, ElemEncodeRaytracingTlasInstances_WithBlasNotAccelerationStructure) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto storageBuffer = TestCreateGpuBuffer(graphicsDevice, 1024, ElemGraphicsHeapType_Gpu, ElemGraphicsResourceUsage_Read);

    auto allocationInfos = ElemGetRaytracingTlasInstanceAllocationInfo(graphicsDevice, 10);
    auto instancesStorageBuffer = TestCreateGpuBuffer(graphicsDevice, allocationInfos.SizeInBytes, ElemGraphicsHeapType_GpuUpload, ElemGraphicsResourceUsage_Read);
    ElemRaytracingTlasInstance tlasInstances[10];

    for (uint32_t i = 0; i < 10; i++)
    {
        tlasInstances[i] = (ElemRaytracingTlasInstance)
        {
            .InstanceId = i,
            .InstanceMask = 1,
            .BlasResource = storageBuffer.Buffer
        };
    }
    
    // Act
    auto tlasInstanceData = ElemEncodeRaytracingTlasInstances((ElemRaytracingTlasInstanceSpan) { .Items = tlasInstances, .Length = 10 });

    // Assert
    TestFreeGpuBuffer(storageBuffer);
    TestFreeGpuBuffer(instancesStorageBuffer);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("BlasResouce in Tlas instance should be an acceleration structure.");
    ASSERT_EQ_MSG(tlasInstanceData.Length, 0u, "Tlas Instance length should be equals to 0.");
}

UTEST(Raytracing, BuildRaytracingTlas) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    
    auto blasStorageBuffer = TestCreateGpuBuffer(graphicsDevice, 1024, ElemGraphicsHeapType_Gpu, ElemGraphicsResourceUsage_RaytracingAccelerationStructure);
    auto blas = ElemCreateRaytracingAccelerationStructureResource(graphicsDevice, blasStorageBuffer.Buffer, nullptr);

    auto allocationInfos = ElemGetRaytracingTlasInstanceAllocationInfo(graphicsDevice, 10);
    auto instancesStorageBuffer = TestCreateGpuBuffer(graphicsDevice, allocationInfos.SizeInBytes, ElemGraphicsHeapType_GpuUpload, ElemGraphicsResourceUsage_Read);
    ElemRaytracingTlasInstance tlasInstances[10];

    for (uint32_t i = 0; i < 10; i++)
    {
        tlasInstances[i] =
        {
            .InstanceId = i,
            .InstanceMask = 1,
            .BlasResource = blas
        };
    }
    
    auto tlasInstanceData = ElemEncodeRaytracingTlasInstances({ .Items = tlasInstances, .Length = 10 });
    ElemUploadGraphicsBufferData(instancesStorageBuffer.Buffer, 0, tlasInstanceData);

    ElemRaytracingTlasParameters parameters
    {
        .BuildFlags = (ElemRaytracingBuildFlags)(ElemRaytracingBuildFlags_PreferFastTrace | ElemRaytracingBuildFlags_AllowUpdate),
        .InstanceBuffer = instancesStorageBuffer.Buffer
    };

    auto allocationInfo = ElemGetRaytracingTlasAllocationInfo(graphicsDevice, &parameters); 
    auto storageBuffer = TestCreateGpuBuffer(graphicsDevice, allocationInfo.SizeInBytes, ElemGraphicsHeapType_Gpu, ElemGraphicsResourceUsage_RaytracingAccelerationStructure);
    auto scratchBuffer = TestCreateGpuBuffer(graphicsDevice, allocationInfo.ScratchSizeInBytes);

    auto accelerationStructure = ElemCreateRaytracingAccelerationStructureResource(graphicsDevice, storageBuffer.Buffer, nullptr);

    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemBuildRaytracingTlas(commandList, accelerationStructure, scratchBuffer.Buffer, &parameters, nullptr);

    // Assert
    ElemCommitCommandList(commandList);

    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    TestFreeGpuBuffer(instancesStorageBuffer);
    TestFreeGpuBuffer(blasStorageBuffer);
    TestFreeGpuBuffer(storageBuffer);
    TestFreeGpuBuffer(scratchBuffer);
    ElemFreeGraphicsResource(blas, nullptr);
    ElemFreeGraphicsResource(accelerationStructure, nullptr);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(allocationInfo.SizeInBytes, 0u, "SizeInBytes should not be equals to 0.");
}
