#pragma once
#include "PreCompiledHeader.h"
#include "Elemental.h"
#include "MacOSApplicationDelegate.h"

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