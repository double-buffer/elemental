#include "Win32Application.cpp"
#include "Win32Window.cpp"
#include "Win32Inputs.cpp"

// TODO: only Include vulkan headers if needed
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "Graphics/DirectX12GraphicsDevice.cpp"
#include "Graphics/DirectX12CommandList.cpp"
#include "Graphics/DirectX12SwapChain.cpp"
#include "Graphics/DirectX12Resource.cpp"
#include "Graphics/DirectX12ResourceBarrier.cpp"
#include "Graphics/DirectX12Shader.cpp"
#include "Graphics/DirectX12Rendering.cpp"

#include "Graphics/Vulkan/VulkanGraphicsDevice.cpp"
#include "Graphics/Vulkan/VulkanCommandList.cpp"
#include "Graphics/Vulkan/VulkanSwapChain.cpp"
#include "Graphics/Vulkan/VulkanResource.cpp"
#include "Graphics/Vulkan/VulkanResourceBarrier.cpp"
#include "Graphics/Vulkan/VulkanShader.cpp"
#include "Graphics/Vulkan/VulkanRendering.cpp"

#include "Graphics/ShaderReader.cpp"
#include "Graphics/ResourceDeleteQueue.cpp"
#include "Graphics/CommandAllocatorPool.cpp"
#include "Graphics/ResourceBarrier.cpp"
#include "Graphics/GraphicsDevice.cpp"
#include "Graphics/CommandList.cpp"
#include "Graphics/SwapChain.cpp"
#include "Graphics/Resource.cpp"
#include "Graphics/Shader.cpp"
#include "Graphics/Rendering.cpp"
#include "Graphics/UploadBufferPool.cpp"

#include "Inputs/Inputs.cpp"
#include "Inputs/HidDevices.cpp"

#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemDataPool.cpp"
