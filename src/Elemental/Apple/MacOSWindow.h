#pragma once

#include "Elemental.h"

class MacOSWindowDelegate : public NS::WindowDelegate
{
    public:
        MacOSWindowDelegate(ElemWindow window);
        ~MacOSWindowDelegate();

        void windowWillClose(NS::Notification* pNotification ) override;

    private:
        ElemWindow _window;
};

struct MacOSWindowData
{
    NS::SharedPtr<NS::Window> WindowHandle;
    bool IsClosed;
};

struct MacOSWindowDataFull
{
    // TODO: Do we need that?
    uint32_t Width;
    uint32_t Height;
    float_t UIScale;
};

MacOSWindowData* GetMacOSWindowData(ElemWindow window);
MacOSWindowDataFull* GetMacOSWindowDataFull(ElemWindow window);
