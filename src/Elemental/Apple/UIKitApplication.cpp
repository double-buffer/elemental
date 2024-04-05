#include "UIKitApplication.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

MemoryArena ApplicationMemoryArena;
SystemDataPool<UIKitApplicationData, UIKitApplicationDataFull> applicationPool;

void InitApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();
        applicationPool = SystemCreateDataPool<UIKitApplicationData, UIKitApplicationDataFull>(ApplicationMemoryArena, 10);

        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Debug Mode");
        #endif
    }
}

UIKitApplicationData* GetUIKitApplicationData(ElemApplication application)
{
    return SystemGetDataPoolItem(applicationPool, application);
}

UIKitApplicationDataFull* GetUIKitApplicationDataFull(ElemApplication application)
{
    return SystemGetDataPoolItemFull(applicationPool, application);
}

ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler)
{
    if (logHandler)
    {
        SystemRegisterLogHandler(logHandler);
    } 
}

ElemAPI ElemApplication ElemCreateApplication(const char* applicationName)
{
    InitApplicationMemory();

    UIKitApplicationData applicationData = {};
    auto handle = SystemAddDataPoolItem(applicationPool, applicationData); 

    UIKitApplicationDataFull applicationDataFull = {};
    applicationDataFull.Status = ElemApplicationStatus_Active;
    SystemAddDataPoolItemFull(applicationPool, handle, applicationDataFull);
    
    UI::Application* pSharedApplication = UI::Application::sharedApplication();
    //pSharedApplication->setDelegate(applicationDataFull.ApplicationDelegate);

    return handle;
}

ElemAPI void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler)
{
    auto canRun = true;

    while (canRun) 
    {
        auto autoreleasePool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

        //ProcessEvents(application);
        // TODO: Temporary
        canRun = false;
        auto applicationDataFull = GetUIKitApplicationDataFull(application);
        SystemAssert(applicationDataFull);

        if (applicationDataFull->Status == ElemApplicationStatus_Closing)
        {
            canRun = false;
        }
        else
        {
            canRun = runHandler(applicationDataFull->Status);
        }
    }
}

ElemAPI void ElemFreeApplication(ElemApplication application)
{
    auto applicationDataFull = GetUIKitApplicationDataFull(application);
    SystemAssert(applicationDataFull);

    SystemRemoveDataPoolItem(applicationPool, application);
}

