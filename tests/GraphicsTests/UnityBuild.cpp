#include "GraphicsDeviceTests.cpp"
#include "CommandListTests.cpp"
#include "utest.h"

bool testForceVulkanApi = false;
bool testHasLogErrors = false;

UTEST_STATE();

int main(int argc, const char *const argv[]) 
{
    if (argc > 1 && strcmp(argv[1], "--vulkan") == 0)
    {
        testForceVulkanApi = true;
    }

    InitLog();
    auto initGraphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto returnCode = utest_main(argc, argv);
    ElemFreeGraphicsDevice(initGraphicsDevice);

    return returnCode;
}
