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

void InitMetal()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    // TODO: There is nothing in code to enable that 😢

    if (MetalDebugLayerEnabled)
    {
        auto sdkLayerExists = false;

        if (sdkLayerExists)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Metal Debug Mode");
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

uint32_t CreateMetalArgumentBufferTextureHandle(MetalArgumentBuffer argumentBuffer, MTL::Texture* texture)
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
    
    // TODO: No debug message queue on metal 😞 
    
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
