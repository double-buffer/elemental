#include "GraphicsDeviceTests.cpp"
#include "CommandListTests.cpp"
#include "ShaderTests.cpp"
#include "SwapChainTests.cpp"
#include "ResourceTests.cpp"
#include "RenderingTests.cpp"
#include "utest.h"

bool testPrintLogs = true;
bool testForceVulkanApi = false;
bool testHasLogErrors = false;
char testLogs[2048];
uint32_t currentTestLogsIndex = 0u;
ElemGraphicsDevice sharedGraphicsDevice = ELEM_HANDLE_NULL;
ElemSystemInfo sharedSystemInfo;
ElemGraphicsDeviceInfo sharedGraphicsDeviceInfo;

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

    options.EnableDebugLayer = true;

    ElemConfigureLogHandler(TestLogHandler);
    ElemSetGraphicsOptions(&options);
    sharedGraphicsDevice = ElemCreateGraphicsDevice(nullptr);
    sharedSystemInfo = ElemGetSystemInfo();
    sharedGraphicsDeviceInfo = ElemGetGraphicsDeviceInfo(sharedGraphicsDevice);

    auto result = utest_main(applicationTestPayload->argc, applicationTestPayload->argv);

    ElemFreeGraphicsDevice(sharedGraphicsDevice);
    ElemExitApplication(result);
}

UTEST_STATE();

int main(int argc, const char* argv[]) 
{
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
