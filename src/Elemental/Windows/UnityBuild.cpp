#include "CircularList.h"

#include "Win32Application.cpp"
#include "Win32Window.cpp"

// TODO: only Include vulkan headers if needed
#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "Graphics/GraphicsDevice.cpp"

#include "Graphics/Direct3D12/Direct3D12GraphicsDevice.cpp"

#include "Vulkan/VulkanGraphicsDevice.cpp"

#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemInputs.cpp"
#include "SystemDataPool.cpp"
