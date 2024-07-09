#include "GraphicsTests.cpp"
#include "GraphicsDeviceTests.cpp"
#include "CommandListTests.cpp"
#include "ShaderTests.cpp"
#include "SwapChainTests.cpp"
#include "ResourceTests.cpp"
#include "ResourceBarrierTests.cpp"
#include "RenderingTests.cpp"
#include "utest.h"

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
    options.EnableGpuValidation = false;
    options.EnableDebugBarrierInfo = true;

    ElemConfigureLogHandler(TestLogHandler);
    ElemSetGraphicsOptions(&options);

    //auto testGraphicsDevice = ElemCreateGraphicsDevice(nullptr);

    auto result = utest_main(applicationTestPayload->argc, applicationTestPayload->argv);

    //ElemFreeGraphicsDevice(testGraphicsDevice);
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
