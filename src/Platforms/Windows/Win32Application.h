#pragma once
#include "WindowsCommon.h"
#include "../Common/Elemental.h"

struct Win32Application
{
    HINSTANCE ApplicationInstance;
    NativeApplicationStatus Status;

    Win32Application()
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

