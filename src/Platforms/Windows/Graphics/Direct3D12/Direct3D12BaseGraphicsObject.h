#pragma once
#include "WindowsCommon.h"
#include "../../../Common/BaseGraphicsObject.h"

struct Direct3D12BaseGraphicsObject : BaseGraphicsObject
{
    Direct3D12BaseGraphicsObject(BaseGraphicsService* graphicsService, ComPtr<ID3D12Device10> device) : BaseGraphicsObject(graphicsService)
    {
        Device = device;
    }

    ComPtr<ID3D12Device10> Device;
};