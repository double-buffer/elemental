#pragma once

#include "Elemental.h"

struct UIKitWindowData
{
    NS::SharedPtr<UI::Window> WindowHandle;
    NS::SharedPtr<UI::ViewController> ViewController;
};

UIKitWindowData* GetUIKitWindowData(ElemWindow window);
