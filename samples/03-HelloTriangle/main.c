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

int main(int argc, const char* argv[]) 
{
    bool useVulkan = false;

    if (argc > 1 && strcmp(argv[1], "--vulkan") == 0)
    {
        useVulkan = true;
    }

    ElemConfigureLogHandler(ElemConsoleLogHandler);
    
    ElemApplication application = ElemCreateApplication("Hello Triangle");
    globalWindow = ElemCreateWindow(application, NULL);

    ElemEnableGraphicsDebugLayer();

    ElemGraphicsDeviceInfo* selectedGraphicsDevice = NULL;
    ElemGraphicsDeviceInfoList graphicsDevices = ElemGetAvailableGraphicsDevices();

    for (uint32_t i = 0; i < graphicsDevices.Length; i++)
    {
        if (graphicsDevices.Items[i].GraphicsApi != ElemGraphicsApi_Vulkan && useVulkan)
        {
            continue;
        }

        printf("GraphicsDevice %s\n", graphicsDevices.Items[i].DeviceName);
        selectedGraphicsDevice = &graphicsDevices.Items[i];
        break;
    }
    
    if (!selectedGraphicsDevice)
    {
        return 1;
    }

    uint64_t deviceId = selectedGraphicsDevice->DeviceId;

    char temp[255];
    sprintf(temp, "Hello Triangle! (GraphicsDevice: DeviceName=%s, GraphicsApi=%s, DeviceId=%llu, AvailableMemory=%llu)", 
                        selectedGraphicsDevice->DeviceName, 
                        GetGraphicsApiLabel(selectedGraphicsDevice->GraphicsApi),
                        selectedGraphicsDevice->DeviceId, 
                        selectedGraphicsDevice->AvailableMemory);
    ElemSetWindowTitle(globalWindow, temp);

    ElemGraphicsDevice graphicsDevice = ElemCreateGraphicsDevice(&(ElemGraphicsDeviceOptions) { .DeviceId = deviceId });

    ElemRunApplication(application, RunHandler);
    ElemFreeApplication(application);

    return 0;
}
