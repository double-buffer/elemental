#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

ElemVector3 TestMipColors[] = 
{
    { 1.0f, 0.0f, 0.0f }, 
    { 1.0f, 1.0f, 0.0f }, 
    { 0.0f, 1.0f, 1.0f }, 
    { 0.0f, 1.0f, 0.0f }, 
    { 0.0f, 0.0f, 1.0f }, 
};

// TODO: File source
// TODO: Big source (so that the upload heap can grow)
// TODO: Test buffer offset and length if too much

UTEST(ResourceIO, UploadGraphicsBufferData_WithBuffer) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_GpuUpload
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), &options);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    uint8_t data[] = { 1, 2, 3, 4 };


    // Act
    ElemUploadGraphicsBufferData(resource, 0, { .Items = data, .Length = ARRAYSIZE(data) });

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
}

UTEST(ResourceIO, UploadGraphicsBufferData_WithNoGpuUploadHeap) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_Gpu
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), &options);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    uint8_t data[] = { 1, 2, 3, 4 };


    // Act
    ElemUploadGraphicsBufferData(resource, 0, { .Items = data, .Length = ARRAYSIZE(data) });

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("UploadGraphicsBufferData only works with graphics buffers allocated in a GpuUpload heap.");
}

UTEST(ResourceIO, UploadGraphicsBufferData_WithBufferOffsetSize) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_GpuUpload
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), &options);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    uint8_t data[] = { 1, 2, 3, 4 };
    ElemUploadGraphicsBufferData(resource, 2, { .Items = data, .Length = 2 });

    // Act
    auto resourceDataSpan = ElemDownloadGraphicsBufferData(resource, nullptr);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    ASSERT_TRUE_MSG(resourceDataSpan.Items != nullptr, "Resource dataspan pointer should not be null.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[0], 0, "Resource dataspan element 0 should be equal to 0.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[1], 0, "Resource dataspan element 1 should be equal to 0.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[2], data[0], "Resource dataspan element 2 should be equal to test data at offset 0.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[3], data[1], "Resource dataspan element 3 should be equal to test data at offset 1.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[4], 0, "Resource dataspan element 2 should be equal to 0.");
}

UTEST(ResourceIO, UploadGraphicsBufferData_WithTexture2D) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    uint8_t data[] = { 1, 2, 3, 4 };

    // Act
    ElemUploadGraphicsBufferData(resource, 0, { .Items = data, .Length = ARRAYSIZE(data) });

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("UploadGraphicsBufferData only works with graphics buffers.");
}

UTEST(ResourceIO, DownloadGraphicsBufferData_WithBuffer) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_GpuUpload
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), &options);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto resourceDataSpan = ElemDownloadGraphicsBufferData(resource, nullptr);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    ASSERT_TRUE_MSG(resourceDataSpan.Items != nullptr, "Resource dataspan pointer should not be null.");
    ASSERT_EQ_MSG(resourceDataSpan.Length, 1024u, "Resource dataspan length should be equals to buffer size.");
}

UTEST(ResourceIO, DownloadGraphicsBufferData_WithNoReadbackHeap) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_Gpu
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), &options);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    ElemDownloadGraphicsBufferData(resource, nullptr);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("DownloadGraphicsBufferData only works with graphics buffers allocated in a Readback heap or GpuUpload heap.");
}

UTEST(ResourceIO, DownloadGraphicsBufferData_WithBufferOffsetAndSize) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_GpuUpload
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), &options);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    uint8_t data[] = { 1, 2, 3, 4 };
    ElemUploadGraphicsBufferData(resource, 0, { .Items = data, .Length = ARRAYSIZE(data) });

    // Act
    ElemDownloadGraphicsBufferDataOptions downloadOptions =
    {
        .Offset = 2,
        .SizeInBytes = 2
    };

    auto resourceDataSpan = ElemDownloadGraphicsBufferData(resource, &downloadOptions);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    ASSERT_TRUE_MSG(resourceDataSpan.Items != nullptr, "Resource dataspan pointer should not be null.");
    ASSERT_EQ_MSG(resourceDataSpan.Length, 2u, "Resource dataspan length should be equals to specified SizeInBytes.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[0], data[2], "Resource dataspan element 0 should be equal to test data at offset 2.");
}

UTEST(ResourceIO, DownloadGraphicsBufferData_WithTexture2D) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto resourceDataSpan = ElemDownloadGraphicsBufferData(resource, nullptr);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("ElemDownloadGraphicsBufferData only works with graphics buffers.");

    ASSERT_TRUE_MSG(resourceDataSpan.Items == nullptr, "Resource dataspan pointer should be null.");
    ASSERT_EQ_MSG(resourceDataSpan.Length, 0u, "Resource dataspan length should be 0.");
}

UTEST(ResourceIO, CopyDataToGraphicsResource_WithBuffer) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_GpuUpload
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), &options);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    uint8_t data[] = { 1, 2, 3, 4 };

    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemCopyDataToGraphicsResourceParameters parameters =
    {
        .Resource = resource,
        .BufferOffset = 2,
        .SourceType = ElemCopyDataSourceType_Memory,
        .SourceMemoryData = { .Items = data, .Length = 2 } 
    };

    ElemCopyDataToGraphicsResource(commandList, &parameters);

    // Assert
    ElemCommitCommandList(commandList);

    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto resourceDataSpan = ElemDownloadGraphicsBufferData(resource, nullptr);

    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    ASSERT_TRUE_MSG(resourceDataSpan.Items != nullptr, "Resource dataspan pointer should not be null.");

    ASSERT_EQ_MSG(resourceDataSpan.Items[0], 0, "Resource dataspan element 0 should be equal to 0.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[1], 0, "Resource dataspan element 1 should be equal to 0.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[2], data[0], "Resource dataspan element 2 should be equal to test data at offset 0.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[3], data[1], "Resource dataspan element 3 should be equal to test data at offset 1.");
    ASSERT_EQ_MSG(resourceDataSpan.Items[4], 0, "Resource dataspan element 2 should be equal to 0.");
}

// TODO: Do the same test but with the copy operations spread accross multiple frames
// TODO: Bigger item count to check the error
UTEST(ResourceIO, CopyDataToGraphicsResource_WithBufferMultiCopies) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_Readback
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(256), &options);

    auto itemCount = 20000u;
    auto resourceSize = itemCount * sizeof(uint32_t);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, resourceSize, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    auto data = (uint32_t*)malloc(resourceSize);

    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    for (uint32_t i = 0; i < itemCount; i++)
    {
        for (uint32_t j = 0; j < itemCount; j++)
        {
            data[j] = i;
        }

        ElemCopyDataToGraphicsResourceParameters parameters =
        {
            .Resource = resource,
            .BufferOffset = (uint32_t)(i * sizeof(uint32_t)),
            .SourceType = ElemCopyDataSourceType_Memory,
            .SourceMemoryData = { .Items = (uint8_t*)data, .Length = (uint32_t)(resourceSize - i * sizeof(uint32_t)) } 
        };

        ElemCopyDataToGraphicsResource(commandList, &parameters);
    }

    // Assert
    ElemCommitCommandList(commandList);

    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto resourceDataSpan = ElemDownloadGraphicsBufferData(resource, nullptr);

    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto bufferValues = (uint32_t*)resourceDataSpan.Items;

    for (uint32_t i = 0; i < itemCount; i++)
    {
        ASSERT_EQ_MSG(bufferValues[i], i, "Resource dataspan element should be equal to initial data.");
    }
    
    free(data);
}

UTEST(ResourceIO, CopyDataToGraphicsResource_WithBufferMultiCopiesAndCommandLists) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    ElemGraphicsHeapOptions options =
    {
        .HeapType = ElemGraphicsHeapType_Readback
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(256), &options);

    auto itemCount = 50000u;
    auto resourceSize = itemCount * sizeof(uint32_t);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, resourceSize, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    auto data = (uint32_t*)malloc(resourceSize);

    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    // Act
    ElemFence fence;

    for (uint32_t i = 0; i < itemCount; i++)
    {
        auto commandList = ElemGetCommandList(commandQueue, nullptr);

        for (uint32_t j = 0; j < itemCount; j++)
        {
            data[j] = i;
        }

        ElemCopyDataToGraphicsResourceParameters parameters =
        {
            .Resource = resource,
            .BufferOffset = (uint32_t)(i * sizeof(uint32_t)),
            .SourceType = ElemCopyDataSourceType_Memory,
            .SourceMemoryData = { .Items = (uint8_t*)data, .Length = (uint32_t)(resourceSize - i * sizeof(uint32_t)) } 
        };

        ElemCopyDataToGraphicsResource(commandList, &parameters);
        ElemCommitCommandList(commandList);
        fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    }

    // Assert
    ElemWaitForFenceOnCpu(fence);

    auto resourceDataSpan = ElemDownloadGraphicsBufferData(resource, nullptr);

    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto bufferValues = (uint32_t*)resourceDataSpan.Items;

    for (uint32_t i = 0; i < itemCount; i++)
    {
        ASSERT_EQ_MSG(bufferValues[i], i, "Resource dataspan element should be equal to initial data.");
    }
    
    free(data);
}

UTEST(ResourceIO, CopyDataToGraphicsResource_WithTexture) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);

    const auto width = 1024u;
    const auto height = 1024u;
    const auto mipLevelCount = 11u;

    auto texture = TestCreateGpuTexture(graphicsDevice, width, height, mipLevelCount, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_Read);

    uint8_t* mipData[mipLevelCount];

    for (uint32_t i = 0; i < mipLevelCount; i++)
    {
        auto currentWidth = max(1u, width >> i);
        auto currentHeight = max(1u, height >> i);

        mipData[i] = (uint8_t*)malloc(currentWidth * currentHeight * 16);
        auto testColor = TestMipColors[i % ARRAYSIZE(TestMipColors)];

        for (uint32_t j = 0; j < currentWidth * currentHeight * 4; j += 4)
        {
            ((float*)mipData[i])[j] = testColor.X;
            ((float*)mipData[i])[j + 1] = testColor.Y;
            ((float*)mipData[i])[j + 2] = testColor.Z;
            ((float*)mipData[i])[j + 3] = 1.0f;
        }
    }

    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    for (uint32_t i = 0; i < mipLevelCount; i++)
    {
        auto currentWidth = max(1u, width >> i);
        auto currentHeight = max(1u, height >> i);

        ElemCopyDataToGraphicsResourceParameters parameters =
        {
            .Resource = texture.Texture,
            .TextureMipLevel = i,
            .SourceType = ElemCopyDataSourceType_Memory,
            .SourceMemoryData = { .Items = mipData[i], .Length = currentWidth * currentHeight * 4 } 
        };

        ElemCopyDataToGraphicsResource(commandList, &parameters);
    }

    // Assert
    ElemCommitCommandList(commandList);

    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    ElemDataSpan outputMipData[mipLevelCount];

    for (uint32_t i = 0; i < mipLevelCount; i++)
    {
        auto currentWidth = max(1u, width >> i);
        auto currentHeight = max(1u, height >> i);

        auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, currentWidth * currentHeight * sizeof(float), ElemGraphicsHeapType_Readback);
        uint32_t resourceIdList[] = { (uint32_t)texture.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor, i };
        TestDispatchComputeForShader(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", (currentWidth + 7) / 8, (currentHeight + 7) / 8, 1, &resourceIdList);

        auto readbackBufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);
        outputMipData[i].Items = (uint8_t*)malloc(readbackBufferData.Length);
        outputMipData[i].Length = readbackBufferData.Length;
        memcpy(outputMipData[i].Items, readbackBufferData.Items, readbackBufferData.Length);

        TestFreeGpuBuffer(readbackBuffer);
        ElemProcessGraphicsResourceDeleteQueue(graphicsDevice);
    }

    TestFreeGpuTexture(texture);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);
    ASSERT_LOG_NOERROR();

    for (uint32_t i = 0; i < mipLevelCount; i++)
    {
        auto outputMipDataItem = outputMipData[i];

        for (uint32_t j = 0; j < outputMipDataItem.Length; j++)
        {
            ASSERT_EQ_MSG(outputMipDataItem.Items[j], mipData[i][j], "Test");
        }

        free(mipData[i]);
        free(outputMipData[i].Items);
    }
}
