#include "SystemPlatformFunctions.h"

void* SystemPlatformAllocateMemory(size_t sizeInBytes)
{
    return malloc(sizeInBytes);
}

void SystemPlatformFreeMemory(void* pointer)
{
    free(pointer);
}

void SystemPlatformClearMemory(void* pointer, size_t sizeInBytes)
{
    memset(pointer, 0, sizeInBytes);
}
