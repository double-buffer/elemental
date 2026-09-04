#include "SystemPlatformFunctions.h"

ReadOnlySpan<char> SystemPlatformGetExecutablePath(MemoryArena memoryArena)
{
    uint32_t pathSize = 0;
    _NSGetExecutablePath(nullptr, &pathSize);

    auto path = SystemPushArrayZero<char>(memoryArena, pathSize);

    if (_NSGetExecutablePath(path.Pointer, &pathSize) != 0)
    {
        return ReadOnlySpan<char>();
    }

    return ReadOnlySpan<char>(path.Pointer);
}
