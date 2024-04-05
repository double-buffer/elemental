#pragma once

#include "Elemental.h"

class UIKitWindowView : public UI::View
{
    public:
    UIKitWindowView(ElemWindow window);
        ~UIKitWindowView();

    private:

};

struct UIKitWindowData
{
    NS::SharedPtr<UI::Window> WindowHandle;
};

struct UIKitWindowDataFull
{
    uint32_t Width;
    uint32_t Height;
    float_t UIScale;
    ElemApplication Application;
    bool ClosingCalled;
};

UIKitWindowData* GetUIKitWindowData(ElemWindow window);
UIKitWindowDataFull* GetUIKitWindowDataFull(ElemWindow window);
