#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

class UIKitApplicationDelegate : public UI::ApplicationDelegate
{
    public:
        UIKitApplicationDelegate(const ElemRunApplicationParameters* runParameters);

        bool applicationDidFinishLaunching(UI::Application *pApp, NS::Value *options) override;
        void applicationWillTerminate(UI::Application *pApp) override;

    private:
        const ElemRunApplicationParameters* _runParameters;
};

extern MemoryArena ApplicationMemoryArena;
