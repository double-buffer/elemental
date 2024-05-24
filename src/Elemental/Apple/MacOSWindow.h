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
    bool IsCursorHidden;
    uint32_t reserved;
};

MacOSWindowData* GetMacOSWindowData(ElemWindow window);
MacOSWindowDataFull* GetMacOSWindowDataFull(ElemWindow window);
