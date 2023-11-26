#pragma once

void* SystemPlatformAllocateMemory(size_t sizeInBytes);
void SystemPlatformFreeMemory(void* pointer);
void SystemPlatformClearMemory(void* pointer, size_t sizeInBytes);
void SystemPlatformCopyMemory(void* destination, const void* source, size_t sizeInBytes);
