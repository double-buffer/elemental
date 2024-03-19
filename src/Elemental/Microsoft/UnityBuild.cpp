#include "CircularList.h"

#include "Win32Application.cpp"
#include "Win32Window.cpp"

// TODO: only Include vulkan headers if needed
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "Graphics/DirectX12GraphicsDevice.cpp"
#include "Graphics/DirectX12CommandList.cpp"

#include "Graphics/Vulkan/VulkanGraphicsDevice.cpp"
#include "Graphics/Vulkan/VulkanCommandList.cpp"

#include "Graphics/GraphicsDevice.cpp"
#include "Graphics/CommandList.cpp"


#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemInputs.cpp"
#include "SystemDataPool.cpp"
