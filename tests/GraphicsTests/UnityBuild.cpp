#include "GraphicsDeviceTests.cpp"
#include "utest.h"

bool testForceVulkanApi = false;

UTEST_STATE();

int main(int argc, const char *const argv[]) 
{
    if (argc > 1 && strcmp(argv[1], "--vulkan") == 0)
    {
        testForceVulkanApi = true;
    }
    
    return utest_main(argc, argv);
}
