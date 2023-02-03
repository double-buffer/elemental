#pragma once
#include "Elemental.h"

class BaseGraphicsService
{
public:
    virtual void GetAvailableGraphicsDevices(void* graphicsDevices, int* count) = 0;
    virtual void* CreateGraphicsDevice(GraphicsDeviceOptions options) = 0;
    virtual void FreeGraphicsDevice(void *graphicsDevicePointer) = 0;
    virtual GraphicsDeviceInfo GetGraphicsDeviceInfo(void *graphicsDevicePointer) = 0;
};