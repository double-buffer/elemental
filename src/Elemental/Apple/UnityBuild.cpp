#include "CircularList.h"

#include "MacOSApplication.cpp"
#include "MacOSWindow.cpp"

// TODO: only Include vulkan headers if needed

#define VK_ENABLE_BETA_EXTENSIONS
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "Graphics/MetalGraphicsDevice.cpp"
#include "Graphics/MetalCommandList.cpp"

#include "Graphics/Vulkan/VulkanGraphicsDevice.cpp"
#include "Graphics/Vulkan/VulkanCommandList.cpp"

#include "Graphics/GraphicsDevice.cpp"
#include "Graphics/CommandList.cpp"

#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemDataPool.cpp"
#include "SystemInputs.cpp"

