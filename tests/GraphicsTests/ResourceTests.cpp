#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test all formats
// TODO: Test barriers with compute/render encoders and different parameters
// TODO: Test barriers inside the same encoder and different encoder

UTEST(Resource, CreateBufferGraphicsResource) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto sizeInBytes = 128u;
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Buffer,
        .Width = sizeInBytes,
        .Usage = ElemGraphicsResourceUsage_Uav
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    resourceInfo = ElemGetGraphicsResourceInfo(resource);

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(resource, ELEM_HANDLE_NULL, "Handle should not be null.");

    ASSERT_EQ_MSG(resourceInfo.Type, ElemGraphicsResourceType_Buffer, "Resource Type should be equals.");
    ASSERT_EQ_MSG(resourceInfo.Width, sizeInBytes, "Width should be equals to buffer size.");
    ASSERT_EQ_MSG(resourceInfo.Height, 0u, "Height should be equals to 0.");
    ASSERT_EQ_MSG(resourceInfo.Format, ElemGraphicsFormat_Raw, "Format should be equals to Raw.");
    ASSERT_EQ_MSG(resourceInfo.Usage, ElemGraphicsResourceUsage_Uav, "Usage should match the creation usage.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateBufferGraphicsResource_WidthZero) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Buffer,
        .Width = 0
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    ASSERT_LOG_MESSAGE("GraphicsBuffer width should not be equals to 0.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");

    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateBufferGraphicsResource_UsageRenderTarget) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
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
    ASSERT_LOG_MESSAGE("GraphicsBuffer usage should not be equals to RenderTarget.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");

    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsBufferResourceInfo) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto sizeInBytes = 1024u;

    ElemGraphicsResourceInfoOptions options =
    {
        .Usage = ElemGraphicsResourceUsage_Uav,
        .DebugName = "TestBuffer"
    };

    // Act
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, sizeInBytes, &options);

    // Assert
    ASSERT_LOG_NOERROR();

    ASSERT_EQ_MSG(resourceInfo.Type, ElemGraphicsResourceType_Buffer, "Resource Type should be a Buffer.");
    ASSERT_EQ_MSG(resourceInfo.Width, sizeInBytes, "Width should be equals to sizeInBytes.");
    ASSERT_EQ_MSG(resourceInfo.Height, 0u, "Height should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.MipLevels, 0u, "MipLevels should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Format, ElemGraphicsFormat_Raw, "Format should be equals to raw.");
    ASSERT_GT_MSG(resourceInfo.Alignment, 0u, "Alignment should be greater than 0.");
    ASSERT_GT_MSG(resourceInfo.SizeInBytes, 0u, "SizeInBytes should be greater than 0.");
    ASSERT_EQ_MSG(resourceInfo.Usage, ElemGraphicsResourceUsage_Uav, "Usage should match the creation usage.");
    ASSERT_STREQ_MSG(resourceInfo.DebugName, "TestBuffer", "Debug name should match the creation usage.");
}

UTEST(Resource, CreateTexture2DGraphicsResource) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
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

    ASSERT_LOG_NOERROR();
    ASSERT_NE_MSG(resource, ELEM_HANDLE_NULL, "Handle should not be null.");

    ASSERT_EQ_MSG(resourceInfo.Type, ElemGraphicsResourceType_Texture2D, "Resource Type should be a Texture2D.");
    ASSERT_EQ_MSG(resourceInfo.Width, width, "Width should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Height, height, "Height should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.MipLevels, mipLevels, "MipLevels should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Format, format, "Format should be equals to creation.");
    ASSERT_EQ_MSG(resourceInfo.Usage, ElemGraphicsResourceUsage_Standard, "Usage should match the creation usage.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateTexture2DGraphicsResource_WidthZero) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Texture2D,
        .Width = 0,
    };

    // Act
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Assert
    ASSERT_LOG_MESSAGE("Texture2D width should not be equals to 0.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");

    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateTexture2DGraphicsResource_HeightZero) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
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
    ASSERT_LOG_MESSAGE("Texture2D height should not be equals to 0.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");
    
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateTexture2DGraphicsResource_MipLevelsZero) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
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
    ASSERT_LOG_MESSAGE("Texture2D mipLevels should not be equals to 0.");
    ASSERT_EQ_MSG(resource, ELEM_HANDLE_NULL, "Handle should be null.");

    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateTexture2DResourceInfo) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto width = 128u;
    auto height = 256u;
    auto mipLevels = 5u;
    auto format = ElemGraphicsFormat_B8G8R8A8_SRGB;

    ElemGraphicsResourceInfoOptions options =
    {
        .Usage = ElemGraphicsResourceUsage_RenderTarget,
        .DebugName = "TestTexture2D"
    };

    // Act
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, width, height, mipLevels, format, &options);

    // Assert
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

UTEST(Resource, GetGraphicsResourceDataSpan_WithBuffer) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto resourceDataSpan = ElemGetGraphicsResourceDataSpan(resource);

    // Assert
    ASSERT_LOG_NOERROR();

    ASSERT_TRUE_MSG(resourceDataSpan.Items != nullptr, "Resource dataspan pointer should not be null.");
    ASSERT_EQ_MSG(resourceDataSpan.Length, 1024u, "Resource dataspan length should be equals to buffer size.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, GetGraphicsResourceDataSpan_WithTexture2D) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto resourceDataSpan = ElemGetGraphicsResourceDataSpan(resource);

    // Assert
    ASSERT_LOG_MESSAGE("GetGraphicsResourceDataSpan only works with graphics buffers.");

    ASSERT_TRUE_MSG(resourceDataSpan.Items == nullptr, "Resource dataspan pointer should be null.");
    ASSERT_EQ_MSG(resourceDataSpan.Length, 0u, "Resource dataspan length should be 0.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, FreeGraphicsResource) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    ElemFreeGraphicsResource(resource, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();

    auto afterFreeResourceInfo = ElemGetGraphicsResourceInfo(resource);
    ASSERT_EQ_MSG(afterFreeResourceInfo.Width, 0u, "Width should be equals to 0.");

    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, FreeGraphicsResource_WithFenceNotExecuted) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto commandQueue = TestGetSharedCommandQueue();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    ElemFence fence = { .CommandQueue = commandQueue, .FenceValue = UINT64_MAX }; 
    ElemFreeGraphicsResourceOptions options = { .FencesToWait = { .Items = &fence, .Length = 1 } };

    // Act
    ElemFreeGraphicsResource(resource, &options);

    // Assert
    ASSERT_LOG_NOERROR();

    ElemProcessGraphicsResourceDeleteQueue();
    auto afterFreeResourceInfo = ElemGetGraphicsResourceInfo(resource);
    ASSERT_EQ_MSG(afterFreeResourceInfo.Width, resourceInfo.Width, "Width should be equals to creation info.");

    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, FreeGraphicsResource_WithFenceExecuted) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto commandQueue = TestGetSharedCommandQueue();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    ElemFreeGraphicsResourceOptions options = { .FencesToWait = { .Items = &fence, .Length = 1 } };

    // Act
    ElemFreeGraphicsResource(resource, &options);

    // Assert
    ASSERT_LOG_NOERROR();

    ElemWaitForFenceOnCpu(fence);
    ElemProcessGraphicsResourceDeleteQueue();
    auto afterFreeResourceInfo = ElemGetGraphicsResourceInfo(resource);
    ASSERT_EQ_MSG(afterFreeResourceInfo.Width, 0u, "Width should be equals to 0.");

    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_StandardWithBuffer) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Standard, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();

    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceUsage_Standard, "Usage should be equals to the one used during creation.");

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_UavWithBufferUav) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    ElemGraphicsResourceInfoOptions options = { .Usage = ElemGraphicsResourceUsage_Uav };
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, &options);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Uav, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();

    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceUsage_Uav, "Usage should be equals to the one used during creation.");

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_UavWithBufferNotUav) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    ElemGraphicsResourceInfoOptions options = { .Usage = ElemGraphicsResourceUsage_Standard };
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, &options);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Uav, nullptr);

    // Assert
    ASSERT_LOG_MESSAGE("Resource Descriptor UAV only works with buffer created with UAV usage.");
    ASSERT_EQ_MSG(descriptor, -1, "Descriptor should be -1.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_StandardWithBufferRenderTarget) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024u, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_RenderTarget, nullptr);

    // Assert
    ASSERT_LOG_MESSAGE("Resource Descriptor RenderTarget only works with textures.");
    ASSERT_EQ_MSG(descriptor, -1, "Descriptor should be -1.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_StandardWithTexture2D) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Standard, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();

    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceUsage_Standard, "Usage should be equals to the one used during creation.");

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_UavWithTexture2DUav) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    ElemGraphicsResourceInfoOptions options = { .Usage = ElemGraphicsResourceUsage_Uav };
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, &options);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Uav, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();

    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceUsage_Uav, "Usage should be equals to the one used during creation.");

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_UavWithTexture2DNotUav) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    ElemGraphicsResourceInfoOptions options = { .Usage = ElemGraphicsResourceUsage_Standard };
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, &options);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Uav, nullptr);

    // Assert
    ASSERT_LOG_MESSAGE("Resource Descriptor UAV only works with texture created with UAV usage.");
    ASSERT_EQ_MSG(descriptor, -1, "Descriptor should be -1.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_RenderTargetWithTexture2DRenderTarget) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    ElemGraphicsResourceInfoOptions options = { .Usage = ElemGraphicsResourceUsage_RenderTarget };
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, &options);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_RenderTarget, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();

    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to the one used during creation.");
    ASSERT_EQ_MSG(descriptorInfo.Usage, ElemGraphicsResourceUsage_RenderTarget, "Usage should be equals to the one used during creation.");

    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);
    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, CreateGraphicsResourceDescriptor_RenderTargetWithTexture2DNotRenderTarget) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    ElemGraphicsResourceInfoOptions options = { .Usage = ElemGraphicsResourceUsage_Uav };
    auto resourceInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 256, 256, 1, ElemGraphicsFormat_B8G8R8A8_SRGB, &options);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);

    // Act
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_RenderTarget, nullptr);

    // Assert
    ASSERT_LOG_MESSAGE("Resource Descriptor RenderTarget only works with texture created with RenderTarget usage.");
    ASSERT_EQ_MSG(descriptor, -1, "Descriptor should be -1.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, FreeGraphicsResourceDescriptor) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Standard, nullptr);

    // Act
    ElemFreeGraphicsResourceDescriptor(descriptor, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();

    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    ASSERT_EQ_MSG(descriptorInfo.Resource, 0u, "Resource should be equals to 0.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, FreeGraphicsResourceDescriptor_WithFenceNotExecuted) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto commandQueue = TestGetSharedCommandQueue();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Standard, nullptr);

    ElemFence fence = { .CommandQueue = commandQueue, .FenceValue = UINT64_MAX }; 
    ElemFreeGraphicsResourceDescriptorOptions options = { .FencesToWait = { .Items = &fence, .Length = 1 } };

    // Act
    ElemFreeGraphicsResourceDescriptor(descriptor, &options);

    // Assert
    ASSERT_LOG_NOERROR();

    ElemProcessGraphicsResourceDeleteQueue();
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    ASSERT_EQ_MSG(descriptorInfo.Resource, resource, "Resource should be equals to creation.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
}

UTEST(Resource, FreeGraphicsResourceDescriptor_WithFenceExecuted) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto commandQueue = TestGetSharedCommandQueue();
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(1), nullptr);
    auto resourceInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, 1024, nullptr);
    auto resource = ElemCreateGraphicsResource(graphicsHeap, 0, &resourceInfo);
    auto descriptor = ElemCreateGraphicsResourceDescriptor(resource, ElemGraphicsResourceUsage_Standard, nullptr);

    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    ElemFreeGraphicsResourceDescriptorOptions options = { .FencesToWait = { .Items = &fence, .Length = 1 } };

    // Act
    ElemFreeGraphicsResourceDescriptor(descriptor, &options);

    // Assert
    ASSERT_LOG_NOERROR();

    ElemWaitForFenceOnCpu(fence);
    ElemProcessGraphicsResourceDeleteQueue();
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    ASSERT_EQ_MSG(descriptorInfo.Resource, 0u, "Resource should be equals to 0.");

    ElemFreeGraphicsResource(resource, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
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
