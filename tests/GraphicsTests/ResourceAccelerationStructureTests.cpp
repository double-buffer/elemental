#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Check when we pass 0 to the buffers size (should be resource size then)
// TODO: Check when passing different flags that memory alloc info is different

UTEST(ResourceAccelerationStructure, GetRaytracingBlasAllocationInfo) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto vertexBuffer = TestCreateGpuBuffer(graphicsDevice, 1024);
    auto indexBuffer = TestCreateGpuBuffer(graphicsDevice, 256);
    auto vertexSizeInBytes = (uint32_t)sizeof(float) * 3u;

    ElemRaytracingBlasParameters parameters
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

    // Act
    auto allocationInfo = ElemGetRaytracingBlasAllocationInfo(graphicsDevice, &parameters); 

    // Assert
    TestFreeGpuBuffer(vertexBuffer);
    TestFreeGpuBuffer(indexBuffer);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(allocationInfo.SizeInBytes, 0u, "SizeInBytes should not be equals to 0.");
}
