#include "Elemental.h"

static ElemWindow globalWindow;

const char* GetGraphicsApiLabel(ElemGraphicsApi graphicsApi)
{
    switch (graphicsApi)
    {
        case ElemGraphicsApi_Direct3D12:
            return "Direct3D12";

        case ElemGraphicsApi_Vulkan:
            return "Vulkan";
    }

    return "Unknown";
}

bool RunHandler(ElemApplicationStatus status)
{
    if (status == ElemApplicationStatus_Closing)
    {
        return false;
    }

    // TODO: Temporary
    #ifdef WIN32
        Sleep(5);
    #else
        usleep(5000);
    #endif

    return true;
}

int main(void)
{
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    
    ElemApplication application = ElemCreateApplication("Hello Triangle");
    globalWindow = ElemCreateWindow(application, NULL);

    ElemEnableGraphicsDebugLayer();

    ElemGraphicsDeviceInfo* selectedGraphicsDevice = NULL;
    ElemGraphicsDeviceInfoList graphicsDevices = ElemGetAvailableGraphicsDevices();

    for (uint32_t i = 0; i < graphicsDevices.Length; i++)
    {
        printf("GraphicsDevice %s\n", graphicsDevices.Items[i].DeviceName);
        selectedGraphicsDevice = &graphicsDevices.Items[i];
    }
    
    if (selectedGraphicsDevice)
    {
        char temp[255];
        sprintf(temp, "Hello Triangle! (GraphicsDevice: DeviceName=%s, GraphicsApi=%s, DeviceId=%llu, AvailableMemory=%llu)", 
                            selectedGraphicsDevice->DeviceName, 
                            GetGraphicsApiLabel(selectedGraphicsDevice->GraphicsApi),
                            selectedGraphicsDevice->DeviceId, 
                            selectedGraphicsDevice->AvailableMemory);
        ElemSetWindowTitle(globalWindow, temp);
    }

    ElemRunApplication(application, RunHandler);
    ElemFreeApplication(application);

    return 0;
}
