#pragma once
#include "WindowsCommon.h"
#include "../../../Common/BaseGraphicsObject.h"

struct Direct3D12CommandQueue : BaseGraphicsObject
{
    Direct3D12CommandQueue(BaseGraphicsService* graphicsService) : BaseGraphicsObject(graphicsService)
    {
    }

    ComPtr<ID3D12CommandQueue> DeviceObject;
};