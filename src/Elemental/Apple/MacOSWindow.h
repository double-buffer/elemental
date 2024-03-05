#pragma once

#include "Elemental.h"

struct MacOSWindowData
{
    NS::SharedPtr<NS::Window> WindowHandle;
};

struct MacOSWindowDataFull
{
    uint32_t Width;
    uint32_t Height;
    float_t UIScale;
};

MacOSWindowData* GetMacOSWindowData(ElemWindow window);
MacOSWindowDataFull* GetMacOSWindowDataFull(ElemWindow window);
