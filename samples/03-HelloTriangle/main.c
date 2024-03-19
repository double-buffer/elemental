#include "Elemental.h"

static ElemWindow globalWindow;
static ElemCommandQueue globalCommandQueue;

const char* GetGraphicsApiLabel(ElemGraphicsApi graphicsApi)
{
    switch (graphicsApi)
    {
        case ElemGraphicsApi_DirectX12:
            return "DirectX12";

        case ElemGraphicsApi_Vulkan:
            return "Vulkan";

        case ElemGraphicsApi_Metal:
            return "Metal";
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

    ElemCommandList commandList = ElemCreateCommandList(globalCommandQueue, &(ElemCommandListOptions) { .DebugName = "TestCommandList" }); 

    ElemCommitCommandList(commandList);

    ElemExecuteCommandList(globalCommandQueue, commandList, NULL);

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
    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .PreferVulkan = useVulkan });
    
    ElemApplication application = ElemCreateApplication("Hello Triangle");
    globalWindow = ElemCreateWindow(application, NULL);
    
    ElemGraphicsDevice graphicsDevice = ElemCreateGraphicsDevice(NULL);
    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(graphicsDevice);

    char temp[255];
    sprintf(temp, "Hello Triangle! (GraphicsDevice: DeviceName=%s, GraphicsApi=%s, DeviceId=%llu, AvailableMemory=%llu)", 
                        graphicsDeviceInfo.DeviceName, 
                        GetGraphicsApiLabel(graphicsDeviceInfo.GraphicsApi),
                        graphicsDeviceInfo.DeviceId, 
                        graphicsDeviceInfo.AvailableMemory);
    ElemSetWindowTitle(globalWindow, temp);

    globalCommandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, &(ElemCommandQueueOptions) { .DebugName = "TestCommandQueue" });

    ElemRunApplication(application, RunHandler);

    ElemFreeCommandQueue(globalCommandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);
    ElemFreeApplication(application);

    return 0;
}
