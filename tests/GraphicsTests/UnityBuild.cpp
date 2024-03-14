#include "GraphicsDeviceTests.cpp"
#include "utest.h"

bool testForceVulkanApi = false;

UTEST_STATE();

int main(int argc, const char *const argv[]) 
{
    testForceVulkanApi = false;
    return utest_main(argc, argv);
}
