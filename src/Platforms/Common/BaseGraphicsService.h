#pragma once
#include "Elemental.h"

class BaseGraphicsService
{
public:
    virtual void* CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics) = 0;
    virtual void DeleteGraphicsDevice(void *graphicsDevicePointer) = 0;
    virtual GraphicsDeviceInfo GetGraphicsDeviceInfo(void *graphicsDevicePointer) = 0;
};