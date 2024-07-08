#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Add a test for texture2D uav + rendertarget
// TODO: Test all formats
// TODO: Test barriers with compute/render encoders and different parameters
// TODO: Test barriers inside the same encoder and different encoder
// TODO: Test Write to write barriers
// TODO: Replace all shared functions because it causes wrong test cases

UTEST(Resource, CreateBufferGraphicsResource) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto sizeInBytes = 128u;
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Buffer,
        .Width = sizeInBytes,
        .Usage = ElemGraphicsResourceUsage_Write
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    resourceInfo = ElemGetGraphicsResourceInfo(resource);

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(resource, ELEM_HANDLE_NULL, "Handle should not be null.");

    ASSERT_EQ_MSG(resourceInfo.Type, ElemGraphicsResourceType_Buffer, "Resource Type should be equals.");
    ASSERT_EQ_MSG(resourceInfo.Width, sizeInBytes, "Width should be equals to buffer size.");
    ASSERT_EQ_MSG(resourceInfo.Height, 0u, "Height should be equals to 0.");
    ASSERT_EQ_MSG(resourceInfo.Format, ElemGraphicsFormat_Raw, "Format should be equals to Raw.");
    ASSERT_EQ_MSG(resourceInfo.Usage, ElemGraphicsResourceUsage_Write, "Usage should match the creation usage.");
}

UTEST(Resource, CreateBufferGraphicsResource_WidthZero) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Buffer,
        .Width = 0
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("GraphicsBuffer width should not be equals to 0.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");
}

UTEST(Resource, CreateBufferGraphicsResource_UsageRenderTarget) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Buffer,
        .Width = 64,
        .Usage = ElemGraphicsResourceUsage_RenderTarget
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("GraphicsBuffer usage should not be equals to RenderTarget.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");
}

UTEST(Resource, CreateBufferGraphicsResource_UsageRenderTargetAndWrite) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Buffer,
        .Width = 64,
        .Usage = (ElemGraphicsResourceUsage)(ElemGraphicsResourceUsage_RenderTarget | ElemGraphicsResourceUsage_Write)
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("GraphicsBuffer usage should not be equals to RenderTarget.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");
}

UTEST(Resource, CreateGraphicsBufferResourceInfo) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto sizeInBytes = 1024u;
    ElemGraphicsResourceInfoOptions options = { .DebugName = "TestBuffer" };

    // Act
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, sizeInBytes, ElemGraphicsResourceUsage_Write, &options);

    // Assert
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    ASSERT_EQ_MSG(resourceInfo.Type, ElemGraphicsResourceType_Buffer, "Resource Type should be a Buffer.");
    ASSERT_EQ_MSG(resourceInfo.Width, sizeInBytes, "Width should be equals to sizeInBytes.");
    ASSERT_EQ_MSG(resourceInfo.Height, 0u, "Height should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.MipLevels, 0u, "MipLevels should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Format, ElemGraphicsFormat_Raw, "Format should be equals to raw.");
    ASSERT_GT_MSG(resourceInfo.Alignment, 0u, "Alignment should be greater than 0.");
    ASSERT_GT_MSG(resourceInfo.SizeInBytes, 0u, "SizeInBytes should be greater than 0.");
    ASSERT_EQ_MSG(resourceInfo.Usage, ElemGraphicsResourceUsage_Write, "Usage should match the creation usage.");
    ASSERT_STREQ_MSG(resourceInfo.DebugName, "TestBuffer", "Debug name should match the creation usage.");
}

UTEST(Resource, CreateTexture2DGraphicsResource) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto width = 128u;
    auto height = 256u;
    auto format = ElemGraphicsFormat_B8G8R8A8_SRGB;
    auto mipLevels = 3u;
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Texture2D,
        .Width = width,
        .Height = height,
        .MipLevels = mipLevels,
        .Format = format,
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    resourceInfo = ElemGetGraphicsResourceInfo(resource);

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(resource, ELEM_HANDLE_NULL, "Handle should not be null.");

    ASSERT_EQ_MSG(resourceInfo.Type, ElemGraphicsResourceType_Texture2D, "Resource Type should be a Texture2D.");
    ASSERT_EQ_MSG(resourceInfo.Width, width, "Width should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Height, height, "Height should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.MipLevels, mipLevels, "MipLevels should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Format, format, "Format should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Usage, ElemGraphicsResourceUsage_Read, "Usage should match the creation usage.");
}

UTEST(Resource, CreateTexture2DGraphicsResource_WidthZero) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Texture2D,
        .Width = 0,
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("Texture2D width should not be equals to 0.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");
}

UTEST(Resource, CreateTexture2DGraphicsResource_HeightZero) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Texture2D,
        .Width = 256,
        .Height = 0,
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("Texture2D height should not be equals to 0.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");
}

UTEST(Resource, CreateTexture2DGraphicsResource_MipLevelsZero) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Texture2D,
        .Width = 256,
        .Height = 256,
        .MipLevels = 0
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("Texture2D mipLevels should not be equals to 0.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");
}

UTEST(Resource, CreateTexture2DResourceInfo) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto width = 128u;
    auto height = 256u;
    auto mipLevels = 5u;
    auto format = ElemGraphicsFormat_B8G8R8A8_SRGB;
    ElemGraphicsResourceInfoOptions options = { .DebugName = "TestTexture2D" };

    // Act
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, width, height, mipLevels, format, ElemGraphicsResourceUsage_RenderTarget, &options);

    // Assert
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    ASSERT_EQ_MSG(resourceInfo.Type, ElemGraphicsResourceType_Texture2D, "Resource Type should be a Texture2D.");
    ASSERT_EQ_MSG(resourceInfo.Width, width, "Width should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Height, height, "Height should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.MipLevels, mipLevels, "MipLevels should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Format, format, "Format should be equals to creation.");
    ASSERT_GT_MSG(resourceInfo.Alignment, 0u, "Alignment should be greater than 0.");
    ASSERT_GT_MSG(resourceInfo.SizeInBytes, 0u, "SizeInBytes should be greater than 0.");
    ASSERT_EQ_MSG(resourceInfo.Usage, ElemGraphicsResourceUsage_RenderTarget, "Usage should match the creation usage.");
    ASSERT_STREQ_MSG(resourceInfo.DebugName, "TestTexture2D", "Debug name should match the creation usage.");
}

UTEST(Resource, CreateTexture2DResourceInfo_RenderTargetWrite) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto width = 128u;
    auto height = 256u;
    auto mipLevels = 5u;
    auto format = ElemGraphicsFormat_B8G8R8A8_SRGB;
    ElemGraphicsResourceInfoOptions options = { .DebugName = "TestTexture2D" };

    // Act
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, width, height, mipLevels, format, (ElemGraphicsResourceUsage)(ElemGraphicsResourceUsage_RenderTarget | ElemGraphicsResourceUsage_Write), &options);

    // Assert
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    ASSERT_EQ_MSG(resourceInfo.Type, ElemGraphicsResourceType_Texture2D, "Resource Type should be a Texture2D.");
    ASSERT_EQ_MSG(resourceInfo.Width, width, "Width should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Height, height, "Height should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.MipLevels, mipLevels, "MipLevels should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Format, format, "Format should be equals to creation.");
    ASSERT_GT_MSG(resourceInfo.Alignment, 0u, "Alignment should be greater than 0.");
    ASSERT_GT_MSG(resourceInfo.SizeInBytes, 0u, "SizeInBytes should be greater than 0.");
    ASSERT_EQ_MSG(resourceInfo.Usage, (ElemGraphicsResourceUsage)(ElemGraphicsResourceUsage_RenderTarget | ElemGraphicsResourceUsage_Write), "Usage should match the creation usage.");
    ASSERT_STREQ_MSG(resourceInfo.DebugName, "TestTexture2D", "Debug name should match the creation usage.");
}

UTEST(Resource, GetGraphicsResourceDataSpan_WithBuffer) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto resourceDataSpan = ElemGetGraphicsResourceDataSpan(resource);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    ASSERT_TRUE_MSG(resourceDataSpan.Items != nullptr, "Resource dataspan pointer should not be null.");
    ASSERT_EQ_MSG(resourceDataSpan.Length, 1024u, "Resource dataspan length should be equals to buffer size.");
}

UTEST(Resource, GetGraphicsResourceDataSpan_WithTexture2D) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto resourceDataSpan = ElemGetGraphicsResourceDataSpan(resource);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("GetGraphicsResourceDataSpan only works with graphics buffers.");

    ASSERT_TRUE_MSG(resourceDataSpan.Items == nullptr, "Resource dataspan pointer should be null.");
    ASSERT_EQ_MSG(resourceDataSpan.Length, 0u, "Resource dataspan length should be 0.");
}

UTEST(Resource, FreeGraphicsResource) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    ElemFreeGraphicsResource(resource, nullptr);

    // Assert
    auto afterFreeResourceInfo = ElemGetGraphicsResourceInfo(resource);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(afterFreeResourceInfo.Width, 0u, "Width should be equals to 0.");
}

UTEST(Resource, FreeGraphicsResource_WithFenceNotExecuted) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    ElemFence fence = { .CommandQueue = commandQueue, .FenceValue = UINT64_MAX }; 
    ElemFreeGraphicsResourceOptions options = { .FencesToWait = { .Items = &fence, .Length = 1 } };

    // Act
    ElemFreeGraphicsResource(resource, &options);

    // Assert
    ElemProcessGraphicsResourceDeleteQueue();
    auto afterFreeResourceInfo = ElemGetGraphicsResourceInfo(resource);
    
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(afterFreeResourceInfo.Width, resourceInfo.Width, "Width should be equals to creation info.");
}

UTEST(Resource, FreeGraphicsResource_WithFenceExecuted) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    ElemFreeGraphicsResourceOptions options = { .FencesToWait = { .Items = &fence, .Length = 1 } };

    // Act
    ElemFreeGraphicsResource(resource, &options);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    ElemProcessGraphicsResourceDeleteQueue();

    auto afterFreeResourceInfo = ElemGetGraphicsResourceInfo(resource);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(afterFreeResourceInfo.Width, 0u, "Width should be equals to 0.");
}

UTEST(Resource, CreateGraphicsResourceDescriptor_ReadWithBuffer) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Read, nullptr);

    // Assert
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    
    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceDescriptorUsage_Read, "Usage should be equals to the one used during creation.");
}

UTEST(Resource, CreateGraphicsResourceDescriptor_WriteWithBufferWrite) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Write, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    // Assert
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);
    
    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceDescriptorUsage_Write, "Usage should be equals to the one used during creation.");
}

UTEST(Resource, CreateGraphicsResourceDescriptor_WriteWithBufferNotWrite) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("Resource Descriptor write only works with buffer created with write usage.");
    ASSERT_EQ_MSG(descriptor, -1, "Descriptor should be -1.");
}

UTEST(Resource, CreateGraphicsResourceDescriptor_ReadWithTexture2D) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Read, nullptr);

    // Assert
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceDescriptorUsage_Read, "Usage should be equals to the one used during creation.");
}

UTEST(Resource, CreateGraphicsResourceDescriptor_WriteWithTexture2DWrite) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, ElemGraphicsResourceUsage_Write, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    // Assert
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceDescriptorUsage_Write, "Usage should be equals to the one used during creation.");
}

UTEST(Resource, CreateGraphicsResourceDescriptor_WriteWithTexture2DNotWrite) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    // Assert
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);
    
    ASSERT_LOG_MESSAGE("Resource Descriptor write only works with texture created with write usage.");
    ASSERT_EQ_MSG(descriptor, -1, "Descriptor should be -1.");
}

UTEST(Resource, FreeGraphicsResourceDescriptor) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Read, nullptr);

    // Act
    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);

    // Assert
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(descriptorInfo.Resource, 0u, "Resource should be equals to 0.");
}

UTEST(Resource, FreeGraphicsResourceDescriptor_WithFenceNotExecuted) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Read, nullptr);

    ElemFence fence = { .CommandQueue = commandQueue, .FenceValue = UINT64_MAX }; 
    ElemFreeGraphicsResourceDescriptorOptions options = { .FencesToWait = { .Items = &fence, .Length = 1 } };

    // Act
    ElemFreeGraphicsResourceDescriptor(descriptor, &options);

    // Assert
    ElemProcessGraphicsResourceDeleteQueue();
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to creation.");
}

UTEST(Resource, FreeGraphicsResourceDescriptor_WithFenceExecuted) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, ElemGraphicsResourceUsage_Read, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceDescriptorUsage_Read, nullptr);

    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    ElemFreeGraphicsResourceDescriptorOptions options = { .FencesToWait = { .Items = &fence, .Length = 1 } };

    // Act
    ElemFreeGraphicsResourceDescriptor(descriptor, &options);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    ElemProcessGraphicsResourceDeleteQueue();
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ_MSG(descriptorInfo.Resource, 0u, "Resource should be equals to 0.");
}

UTEST(Resource, FreeGraphicsResourceDescriptor_WithInvalidDescriptor) 
{
    // Arrange
    ElemGraphicsResourceDescriptor descriptor = -1;

    // Act
    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);

    // Assert
    ASSERT_LOG_MESSAGE("Resource Descriptor is invalid.");
}

UTEST(Resource, GetGraphicsResourceDescriptorInfo_WithInvalidDescriptor) 
{
    // Arrange
    ElemGraphicsResourceDescriptor descriptor = -1;

    // Act
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);

    // Assert
    ASSERT_LOG_MESSAGE("Resource Descriptor is invalid.");
    ASSERT_EQ_MSG(descriptorInfo.Resource, 0u, "Resource should be equals to 0.");
}

UTEST(Resource, GraphicsResourceBarrier_BufferReadAfterWrite) 
{
    // Arrange
    int32_t elementCount = 1000000;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(8), nullptr);

    auto gpuBufferInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsResourceUsage_Write, nullptr);
    auto gpuBuffer = ElemCreateGraphicsResource(graphicsHeap, 0, &gpuBufferInfo);
    auto gpuBufferReadDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Read, nullptr);
    auto gpuBufferWriteDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    auto readbackBuffer = TestCreateReadbackBuffer(graphicsDevice, elementCount * sizeof(uint32_t));

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceTests.shader", "TestWriteBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceTests.shader", "TestReadBufferData");

    int32_t writeParameters[] = { gpuBufferWriteDescriptor, 0, elementCount };
    int32_t readParameters[] = { gpuBufferReadDescriptor, readbackBuffer.Descriptor, elementCount };

    uint32_t threadSize = 16;

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBufferWriteDescriptor, nullptr);

    ElemBindPipelineState(commandList, writeBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)writeParameters, .Length = ARRAYSIZE(writeParameters) * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer, SyncType_None, SyncType_Compute, AccessType_NoAccess, AccessType_Write)
    ));

    ElemGraphicsResourceBarrier(commandList, gpuBufferReadDescriptor, nullptr);

    ElemBindPipelineState(commandList, readBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)readParameters, .Length = ARRAYSIZE(readParameters) * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read)
    ));

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeReadbackBuffer(readbackBuffer);
    ElemFreeGraphicsResource(gpuBuffer, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);

    auto intData = (int32_t*)bufferData.Items;

    for (int32_t i = 0; i < elementCount; i++)
    {
        ASSERT_EQ_MSG(intData[i], elementCount - i - 1, "Compute shader data is invalid.");
    }
}

UTEST(Resource, GraphicsResourceBarrier_BufferWriteAfterWrite) 
{
    // Arrange
    int32_t elementCount = 1000000;
    int32_t addOffset = 28;

    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(8), nullptr);

    auto gpuBufferInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsResourceUsage_Write, nullptr);
    auto gpuBuffer = ElemCreateGraphicsResource(graphicsHeap, 0, &gpuBufferInfo);
    auto gpuBufferReadDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Read, nullptr);
    auto gpuBufferWriteDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    auto readbackBuffer = TestCreateReadbackBuffer(graphicsDevice, elementCount * sizeof(uint32_t));

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceTests.shader", "TestWriteBufferData");
    auto writeAddBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceTests.shader", "TestWriteAddBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceTests.shader", "TestReadBufferData");

    int32_t writeParameters[3] = { gpuBufferWriteDescriptor, 0, elementCount };
    int32_t writeAddParameters[3] = { gpuBufferWriteDescriptor, addOffset, elementCount };
    int32_t readParameters[3] = { gpuBufferReadDescriptor, readbackBuffer.Descriptor, elementCount };

    uint32_t threadSize = 16;

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBufferWriteDescriptor, nullptr);

    ElemBindPipelineState(commandList, writeBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)writeParameters, .Length = 3 * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);
   
    ElemGraphicsResourceBarrier(commandList, gpuBufferWriteDescriptor, nullptr);

    ElemBindPipelineState(commandList, writeAddBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)writeAddParameters, .Length = 3 * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);

    // TODO: Assert debug barrier

    ElemGraphicsResourceBarrier(commandList, gpuBufferReadDescriptor, nullptr);

    ElemBindPipelineState(commandList, readBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)readParameters, .Length = 3 * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeAddBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeReadbackBuffer(readbackBuffer);
    ElemFreeGraphicsResource(gpuBuffer, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    auto intData = (int32_t*)bufferData.Items;

    for (int32_t i = 0; i < elementCount; i++)
    {
        ASSERT_EQ_MSG(intData[i], elementCount - i - 1 + addOffset, "Compute shader data is invalid.");
    }
}