#include "Elemental.h"
#include "Direct3D12/Direct3D12GraphicsDevice.h"

ElemAPI void ElemEnableGraphicsDebugLayer()
{
    Direct3D12EnableGraphicsDebugLayer();
}

ElemAPI ElemGraphicsDeviceInfoList ElemGetAvailableGraphicsDevices()
{
    return Direct3D12GetAvailableGraphicsDevices();
}
