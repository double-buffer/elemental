#pragma once
#include "Elemental.h"

class BaseGraphicsService
{
public:
    virtual void GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count) = 0;
    virtual void* CreateGraphicsDevice(GraphicsDeviceOptions options) = 0;
    virtual void FreeGraphicsDevice(void* graphicsDevicePointer) = 0;
    virtual GraphicsDeviceInfo GetGraphicsDeviceInfo(void* graphicsDevicePointer) = 0;
    
    virtual void* CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type) = 0;
    virtual void FreeCommandQueue(void* commandQueuePointer) = 0;
    virtual void SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label) = 0;

    virtual void* CreateCommandList(void* commandQueuePointer) = 0;
    virtual void FreeCommandList(void* commandListPointer) = 0;
    virtual void SetCommandListLabel(void* commandListPointer, uint8_t* label) = 0;    
    virtual void CommitCommandList(void* commandList) = 0;
};