#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define METAL_MAXDEVICES 10u

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

    auto handle = SystemAddDataPoolItem(metalGraphicsDevicePool, {
        .Device = device
    }); 

    SystemAddDataPoolItemFull(metalGraphicsDevicePool, handle, {
    });

    return handle;
}

void MetalFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    graphicsDeviceData->Device.reset();
}

ElemGraphicsDeviceInfo MetalGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    return MetalConstructGraphicsDeviceInfo(graphicsDeviceData->Device.get());
}
