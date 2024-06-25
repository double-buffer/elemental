#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define METAL_MAXDEVICES 10u

struct MetalArgumentBufferDescriptor
{
    uint64_t BufferAddress;
    uint64_t TextureResourceId;
    uint64_t Metadata;
};

struct MetalArgumentBufferFreeListItem
{
    uint32_t Next;
};

struct MetalArgumentBufferStorage
{
    NS::SharedPtr<MTL::Buffer> ArgumentBuffer;
    Span<MetalArgumentBufferFreeListItem> Items;
    uint32_t CurrentIndex;
    uint32_t FreeListIndex;
};

MemoryArena MetalGraphicsMemoryArena;
SystemDataPool<MetalGraphicsDeviceData, MetalGraphicsDeviceDataFull> metalGraphicsDevicePool;

bool MetalDebugLayerEnabled = false;

void* MetalDebugReportCallback(void* arg) 
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    int fd = *((int*)arg);
    char buffer[256];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) 
    {
        buffer[bytesRead - 1] = '\0';

        auto splittedStrings = SystemSplitString(stackMemoryArena, ReadOnlySpan<char>(buffer), ']');
        if (splittedStrings.Length == 2)
        {
            auto message = ((ReadOnlySpan<char>)splittedStrings[1]).Slice(1);

            if (SystemFindSubString(message, "Metal API Validation Enabled") == -1)
            {
                SystemLogErrorMessage(ElemLogMessageCategory_Graphics, message);
            }
        }
    }

    return nullptr;
}

void InitMetal()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    // TODO: There is nothing in code to enable that 😢

    if (MetalDebugLayerEnabled)
    {
        setenv("MTL_DEBUG_LAYER", "1", 1); 
        setenv("MTL_DEBUG_LAYER_ERROR_MODE", "nslog", 1); 
        setenv("MTL_DEBUG_LAYER_WARNING_MODE", "nslog", 1); 
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Metal Debug Mode");

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return;
        }

        // Redirect stderr to the write end of the pipe
        if (dup2(pipefd[1], fileno(stderr)) == -1) {
            perror("dup2");
            return;
        }

        // Close the write end of the pipe in the parent process
        close(pipefd[1]);

        // Create a thread to read from the pipe
        pthread_t thread;
        if (pthread_create(&thread, nullptr, MetalDebugReportCallback, &pipefd[0]) != 0) 
        {
            perror("pthread_create");
            return;
        }
    }
    else
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Metal..."); 
    }
}

void InitMetalGraphicsDeviceMemory()
{
    if (!MetalGraphicsMemoryArena.Storage)
    {
        MetalGraphicsMemoryArena = SystemAllocateMemoryArena();
        metalGraphicsDevicePool = SystemCreateDataPool<MetalGraphicsDeviceData, MetalGraphicsDeviceDataFull>(MetalGraphicsMemoryArena, METAL_MAXDEVICES);

        InitMetal();
    }
}

ElemGraphicsDeviceInfo MetalConstructGraphicsDeviceInfo(const MTL::Device* device)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto deviceName = ReadOnlySpan<char>(device->name()->utf8String());
    auto destinationDeviceName = SystemPushArray<char>(stackMemoryArena, deviceName.Length);
    SystemCopyBuffer<char>(destinationDeviceName, deviceName);

    return 
    {
        .DeviceName = destinationDeviceName.Pointer,
        .GraphicsApi = ElemGraphicsApi_Metal,
        .DeviceId = device->registryID(),
        .AvailableMemory = device->recommendedMaxWorkingSetSize()
    };
}

bool MetalCheckGraphicsDeviceCompatibility(const MTL::Device* device)
{
    // TODO: This crash on ios simulator 
    return true;//device->supportsRaytracing();
}

MetalGraphicsDeviceData* GetMetalGraphicsDeviceData(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItem(metalGraphicsDevicePool, graphicsDevice);
}

MetalGraphicsDeviceDataFull* GetMetalGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItemFull(metalGraphicsDevicePool, graphicsDevice);
}

MetalArgumentBuffer CreateMetalArgumentBuffer(NS::SharedPtr<MTL::Device> graphicsDevice, uint32_t length)
{
    auto argumentBuffer = NS::TransferPtr(graphicsDevice->newBuffer(sizeof(MetalArgumentBufferDescriptor) * length, MTL::ResourceOptionCPUCacheModeDefault));
    SystemAssert(argumentBuffer);

    auto descriptorStorage = SystemPushStruct<MetalArgumentBufferStorage>(MetalGraphicsMemoryArena);
    descriptorStorage->ArgumentBuffer = argumentBuffer;
    descriptorStorage->Items = SystemPushArray<MetalArgumentBufferFreeListItem>(MetalGraphicsMemoryArena, length);
    descriptorStorage->CurrentIndex = 0;
    descriptorStorage->FreeListIndex = UINT32_MAX;

    return
    {
        .Storage = descriptorStorage
    };
}

void FreeMetalArgumentBuffer(MetalArgumentBuffer descriptorHeap)
{
    SystemAssert(descriptorHeap.Storage);
    descriptorHeap.Storage->ArgumentBuffer.reset();
}

// TODO: Buffers

// TODO: We need to extract the code get or update handles (index) to a common place because 
// we need to be able to manage double buffering if needed for updates
uint32_t CreateMetalArgumentBufferHandleForTexture(MetalArgumentBuffer argumentBuffer, MTL::Texture* texture)
{            
    SystemAssert(argumentBuffer.Storage);

    auto storage = argumentBuffer.Storage;
    auto argumentIndex = UINT32_MAX;

    do
    {
        if (storage->FreeListIndex == UINT32_MAX)
        {
            argumentIndex = UINT32_MAX;
            break;
        }
        
        argumentIndex = storage->FreeListIndex;
    } while (!SystemAtomicCompareExchange(storage->FreeListIndex, argumentIndex, storage->Items[storage->FreeListIndex].Next));

    if (argumentIndex == UINT32_MAX)
    {
        argumentIndex = SystemAtomicAdd(storage->CurrentIndex, 1);
    }

    auto argumentBufferData = (MetalArgumentBufferDescriptor*)storage->ArgumentBuffer->contents();
    argumentBufferData[argumentIndex].TextureResourceId = (uint64_t)texture->gpuResourceID()._impl;

    return argumentIndex;
}

const uint64_t kIRBufSizeOffset     = 0;
const uint64_t kIRBufSizeMask       = 0xffffffff;
const uint64_t kIRTypedBufferOffset = 63;

uint32_t CreateMetalArgumentBufferHandleForBuffer(MetalArgumentBuffer argumentBuffer, MTL::Buffer* buffer, uint32_t length)
{            
    SystemAssert(argumentBuffer.Storage);

    auto storage = argumentBuffer.Storage;
    auto argumentIndex = UINT32_MAX;

    do
    {
        if (storage->FreeListIndex == UINT32_MAX)
        {
            argumentIndex = UINT32_MAX;
            break;
        }
        
        argumentIndex = storage->FreeListIndex;
    } while (!SystemAtomicCompareExchange(storage->FreeListIndex, argumentIndex, storage->Items[storage->FreeListIndex].Next));

    if (argumentIndex == UINT32_MAX)
    {
        argumentIndex = SystemAtomicAdd(storage->CurrentIndex, 1);
    }

    auto argumentBufferData = (MetalArgumentBufferDescriptor*)storage->ArgumentBuffer->contents();
    argumentBufferData[argumentIndex].BufferAddress = (uint64_t)buffer->gpuAddress();

    uint32_t typedBuffer = 1;
    uint64_t md = (length & kIRBufSizeMask) << kIRBufSizeOffset;
    //md |= ((uint64_t)view->textureViewOffsetInElements & kIRTexViewMask) << kIRTexViewOffset;
    md |= (uint64_t)typedBuffer << kIRTypedBufferOffset; 
    
    argumentBufferData[argumentIndex].Metadata = md;

    return argumentIndex;
}

void FreeMetalArgumentBufferHandle(MetalArgumentBuffer argumentBuffer, uint64_t handle)
{
    auto storage = argumentBuffer.Storage;
    
    do
    {
        storage->Items[handle].Next = storage->FreeListIndex;
    } while (!SystemAtomicCompareExchange(storage->FreeListIndex, storage->FreeListIndex, handle));
}

void MetalEnableGraphicsDebugLayer()
{
    MetalDebugLayerEnabled = true;
}

ElemGraphicsDeviceInfoSpan MetalGetAvailableGraphicsDevices()
{
    InitMetalGraphicsDeviceMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto deviceInfos = SystemPushArray<ElemGraphicsDeviceInfo>(stackMemoryArena, METAL_MAXDEVICES);
    auto currentDeviceInfoIndex = 0u;

    auto device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    
    if (MetalCheckGraphicsDeviceCompatibility(device.get()))
    {
        deviceInfos[currentDeviceInfoIndex++] = MetalConstructGraphicsDeviceInfo(device.get());
    }

    return
    {
        .Items = deviceInfos.Pointer,
        .Length = currentDeviceInfoIndex
    };
}

ElemGraphicsDevice MetalCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options)
{
    InitMetalGraphicsDeviceMemory();

    bool foundAdapter = false;
    auto device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    
    if (MetalCheckGraphicsDeviceCompatibility(device.get()))
    {
        foundAdapter = true;
    }

    SystemAssertReturnNullHandle(foundAdapter);
    
    auto resourceArgumentBuffer = CreateMetalArgumentBuffer(device, METAL_MAX_RESOURCES);

    auto residencySetDescriptor = NS::TransferPtr(MTL::ResidencySetDescriptor::alloc()->init());
    residencySetDescriptor->setInitialCapacity(8);

    NS::Error* error;
    auto residencySet = NS::TransferPtr(device->newResidencySet(residencySetDescriptor.get(), &error));
    SystemAssert(error == nullptr);
    
    residencySet->addAllocation(resourceArgumentBuffer.Storage->ArgumentBuffer.get());
    residencySet->commit();

    auto handle = SystemAddDataPoolItem(metalGraphicsDevicePool, {
        .Device = device,
        .ResourceArgumentBuffer = resourceArgumentBuffer
    }); 

    SystemAddDataPoolItemFull(metalGraphicsDevicePool, handle, {
        .ResidencySet = residencySet
    });

    return handle;
}

void MetalFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    FreeMetalArgumentBuffer(graphicsDeviceData->ResourceArgumentBuffer);
    
    graphicsDeviceData->Device.reset();
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Releasing Metal");
}

ElemGraphicsDeviceInfo MetalGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    return MetalConstructGraphicsDeviceInfo(graphicsDeviceData->Device.get());
}
