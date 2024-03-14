#include "GraphicsDeviceTests.cpp"
#include "utest.h"

bool testForceVulkanApi = false;

UTEST_STATE();

int main(int argc, const char *const argv[]) 
{
    printf("Teeeest Main\n");
    testForceVulkanApi = false;
    return utest_main(argc, argv);
}
