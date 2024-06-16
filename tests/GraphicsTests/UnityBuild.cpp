#include "GraphicsDeviceTests.cpp"
#include "CommandListTests.cpp"
#include "SwapChainTests.cpp"
#include "ResourceTests.cpp"
#include "RenderingTests.cpp"
#include "utest.h"

bool testForceVulkanApi = false;
bool testHasLogErrors = false;
char testLogs[2048];
uint32_t currentTestLogsIndex = 0u;
ElemGraphicsDevice sharedGraphicsDevice = ELEM_HANDLE_NULL;

struct ApplicationTestPayload
{
    int argc;
    const char** argv;
};

void ApplicationTestInitFunction(void* payload)
{
    auto applicationTestPayload = (ApplicationTestPayload*)payload;
    ElemGraphicsOptions options = {};

    if (applicationTestPayload->argc > 1 && strcmp(applicationTestPayload->argv[1], "--vulkan") == 0)
    {
        testForceVulkanApi = true;
        options.PreferVulkan = true;
    }

    #ifdef _DEBUG
    ElemConfigureLogHandler(TestLogHandler);
    options.EnableDebugLayer = true;
    #else
    ElemConfigureLogHandler(ElemConsoleErrorLogHandler);
    #endif

    ElemSetGraphicsOptions(&options);
    sharedGraphicsDevice = ElemCreateGraphicsDevice(nullptr);

    auto result = utest_main(applicationTestPayload->argc, applicationTestPayload->argv);

    ElemFreeGraphicsDevice(sharedGraphicsDevice);
    ElemExitApplication(result);
}

UTEST_STATE();

int main(int argc, const char* argv[]) 
{
    #ifdef _DEBUG
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    #endif

    ApplicationTestPayload payload =
    {
        .argc = argc,
        .argv = argv
    };

    ElemRunApplicationParameters runParameters = 
    {
        .InitHandler = ApplicationTestInitFunction,
        .Payload = &payload
    };

    ElemRunApplication(&runParameters);
    return 0;
}
