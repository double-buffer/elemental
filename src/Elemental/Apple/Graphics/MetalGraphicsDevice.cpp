#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define METAL_MAXDEVICES 10u

MemoryArena MetalGraphicsMemoryArena;
SystemDataPool<MetalGraphicsDeviceData, MetalGraphicsDeviceDataFull> metalGraphicsDevicePool;

bool metalDebugLayerEnabled = false;

void InitMetal()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    // TODO: There is nothing in code to enable that 😢

    if (metalDebugLayerEnabled)
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
    // TODO: 
    return device->supportsRaytracing();
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
    metalDebugLayerEnabled = true;
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

    return {};
}

void MetalFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
}

ElemGraphicsDeviceInfo MetalGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    return {};
}
