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
#include "Graphics/MetalSwapChain.cpp"
#include "Graphics/MetalShader.cpp"
#include "Graphics/MetalRendering.cpp"
#include "Graphics/MetalTexture.cpp"

#include "Graphics/Vulkan/VulkanGraphicsDevice.cpp"
#include "Graphics/Vulkan/VulkanCommandList.cpp"
#include "Graphics/Vulkan/VulkanSwapChain.cpp"
#include "Graphics/Vulkan/VulkanShader.cpp"
#include "Graphics/Vulkan/VulkanRendering.cpp"

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

