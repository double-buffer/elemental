#pragma once

extern bool useVulkan;

#ifdef _WIN32
#include "Microsoft/Graphics/DirectX12GraphicsDevice.h"
#include "Graphics/Vulkan/VulkanGraphicsDevice.h"

#define DispatchGraphicsFunction(functionName, ...) \
    if (useVulkan) \
        Vulkan##functionName(__VA_ARGS__); \
    else \
        DirectX12##functionName(__VA_ARGS__);

#define DispatchReturnGraphicsFunction(functionName, ...) \
    if (useVulkan) \
        return Vulkan##functionName(__VA_ARGS__); \
    else \
        return DirectX12##functionName(__VA_ARGS__);
#endif

#ifdef __APPLE__
#include "Apple/Graphics/MetalGraphicsDevice.h"

#define DispatchGraphicsFunction(functionName, ...) \
        Metal##functionName(__VA_ARGS__);

#define DispatchReturnGraphicsFunction(functionName, ...) \
        return Metal##functionName(__VA_ARGS__);
#endif

#ifdef __linux__
#include "Graphics/Vulkan/VulkanGraphicsDevice.h"

#define DispatchGraphicsFunction(functionName, ...) \
        Vulkan##functionName(__VA_ARGS__);

#define DispatchReturnGraphicsFunction(functionName, ...) \
        return Vulkan##functionName(__VA_ARGS__); 

#endif
