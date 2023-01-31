#pragma once
#include "../../../Common/BaseGraphicsService.h"
#include "../StringConverters.h"

class Direct3D12GraphicsService : BaseGraphicsService
{
public:
    void* CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics) override;
    void DeleteGraphicsDevice(void *graphicsDevicePointer) override;
    GraphicsDeviceInfo GetGraphicsDeviceInfo(void *graphicsDevicePointer) override;
};