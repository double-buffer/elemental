#include "GtkApplication.cpp"
#include "GtkWindow.cpp"

#define VK_USE_PLATFORM_WAYLAND_KHR
#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "Graphics/Vulkan/VulkanGraphicsDevice.cpp"
#include "Graphics/Vulkan/VulkanCommandList.cpp"
#include "Graphics/Vulkan/VulkanSwapChain.cpp"
#include "Graphics/Vulkan/VulkanTexture.cpp"
#include "Graphics/Vulkan/VulkanShader.cpp"
#include "Graphics/Vulkan/VulkanRendering.cpp"

#include "Graphics/CommandAllocatorPool.cpp"
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
#include "SystemInputs.cpp"
#include "SystemDataPool.cpp"
