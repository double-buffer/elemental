#include "SystemPlatformFunctions.h"

ReadOnlySpan<char> SystemPlatformGetExecutablePath(MemoryArena memoryArena)
{
    auto path = (ReadOnlySpan<char>)NS::Bundle::mainBundle()->executablePath()->utf8String();
    auto result = SystemPushArrayZero<char>(memoryArena, path.Length);

    SystemCopyBuffer(result, path);

    return result;
}
