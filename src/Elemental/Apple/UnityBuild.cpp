#include "CircularList.h"

#include <TargetConditionals.h>

#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#include "MacOSApplication.cpp"
#include "MacOSWindow.cpp"
#else
#include "UIKitApplication.cpp"
#include "UIKitWindow.cpp"
#endif


#include "Graphics/MetalGraphicsDevice.cpp"
#include "Graphics/MetalCommandList.cpp"
#include "Graphics/MetalSwapChain.cpp"
#include "Graphics/MetalShader.cpp"
#include "Graphics/MetalRendering.cpp"
#include "Graphics/MetalTexture.cpp"

#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
// TODO: Only include vulkan headers if needed by a compile flags so we can make lighter versions
#define VK_ENABLE_BETA_EXTENSIONS
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "Graphics/Vulkan/VulkanGraphicsDevice.cpp"
#include "Graphics/Vulkan/VulkanCommandList.cpp"
#include "Graphics/Vulkan/VulkanSwapChain.cpp"
#include "Graphics/Vulkan/VulkanShader.cpp"
#include "Graphics/Vulkan/VulkanRendering.cpp"
#endif

#include "Graphics/GraphicsDevice.cpp"
#include "Graphics/CommandList.cpp"
#include "Graphics/SwapChain.cpp"
#include "Graphics/Shader.cpp"
#include "Graphics/Rendering.cpp"

#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemDataPool.cpp"
#include "SystemInputs.cpp"

