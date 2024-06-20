#include <TargetConditionals.h>

#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#include "MacOSApplication.cpp"
#include "MacOSWindow.cpp"
#else
#include "UIKitApplication.cpp"
#include "UIKitWindow.cpp"
#endif

#include "Inputs.cpp"

#include "Graphics/MetalGraphicsDevice.cpp"
#include "Graphics/MetalCommandList.cpp"
#include "Graphics/MetalSwapChain.cpp"
#include "Graphics/MetalShader.cpp"
#include "Graphics/MetalRendering.cpp"
#include "Graphics/MetalResource.cpp"

#include "Graphics/ShaderReader.cpp"
#include "Graphics/GraphicsDevice.cpp"
#include "Graphics/CommandList.cpp"
#include "Graphics/SwapChain.cpp"
#include "Graphics/Shader.cpp"
#include "Graphics/Rendering.cpp"
#include "Graphics/Resource.cpp"

#include "Inputs/Inputs.cpp"

#include "PosixPlatformFunctions.cpp"
#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemDataPool.cpp"

