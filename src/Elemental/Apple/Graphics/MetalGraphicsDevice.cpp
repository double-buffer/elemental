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

    if (metalDebugLayerEnabled)
    {
        auto sdkLayerExists = false;

        if (sdkLayerExists)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Metal Debug Mode");
        }
        else
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "D3D12SDKLayers.dll not found but EnableGraphicsDebugLayer() was called. Debug layer will not be enabled."); 
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

/*
ElemGraphicsDeviceInfo MetalConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    return 
    {
        .DeviceName = SystemConvertWideCharToUtf8(stackMemoryArena, adapterDescription.Description).Pointer,
        .GraphicsApi = ElemGraphicsApi_Metal,
        .DeviceId = *(uint64_t *)&adapterDescription.AdapterLuid,
        .AvailableMemory = adapterDescription.DedicatedVideoMemory
    };
}*/

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

ElemGraphicsDeviceInfoList MetalGetAvailableGraphicsDevices()
{
    InitMetalGraphicsDeviceMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto deviceInfos = SystemPushArray<ElemGraphicsDeviceInfo>(stackMemoryArena, METAL_MAXDEVICES);
    auto currentDeviceInfoIndex = 0u;

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
