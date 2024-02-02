#pragma once

#include "ElementalOld.h"

class MacOSApplicationDelegate;

struct MacOSApplication
{
    MacOSApplicationDelegate* ApplicationDelegate;
    NativeApplicationStatus Status;

    MacOSApplication()
    {
        Status.Status = 0;
        SetStatus(NativeApplicationStatusFlags::Active, 1);
    }
    
    bool IsStatusActive(NativeApplicationStatusFlags flag) 
    {
        return (Status.Status & flag) != 0;
    }
    
    void SetStatus(NativeApplicationStatusFlags flag, int value) 
    {
        Status.Status |= value << (flag - 1);
    }
};
