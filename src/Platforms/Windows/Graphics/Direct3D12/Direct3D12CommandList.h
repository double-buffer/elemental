#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12CommandList : Direct3D12BaseGraphicsObject
{
    Direct3D12CommandList(BaseGraphicsService* graphicsService, ComPtr<ID3D12Device10> device) : Direct3D12BaseGraphicsObject(graphicsService, device)
    {
    }

    ComPtr<ID3D12GraphicsCommandList7> DeviceObject;
    D3D12_COMMAND_LIST_TYPE CommandListType;
};