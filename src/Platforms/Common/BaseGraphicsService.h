#pragma once
#include "Elemental.h"

class BaseGraphicsService
{
public:
    virtual void* CreateGraphicsDevice(GraphicsDeviceOptions options) = 0;
    virtual void FreeGraphicsDevice(void *graphicsDevicePointer) = 0;
    virtual GraphicsDeviceInfo GetGraphicsDeviceInfo(void *graphicsDevicePointer) = 0;
};