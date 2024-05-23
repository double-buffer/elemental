#include "WaylandApplication.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemSpan.h"
#include "SystemPlatformFunctions.h"

#define WAYLAND_MAX_INITHANDLER 10u

MemoryArena ApplicationMemoryArena;
Span<WaylandInitHandler> WaylandInitHandlers;
uint32_t WaylandCurrentInitIndex;

uint64_t WaylandPerformanceCounterStart;
uint64_t WaylandPerformanceCounterFrequencyInSeconds;

wl_display* WaylandDisplay = nullptr;
wl_compositor* WaylandCompositor = nullptr;
xdg_wm_base* WaylandShell = nullptr;
bool waylandApplicationCanRun = false;

void InitWaylandApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();
        WaylandInitHandlers = SystemPushArray<WaylandInitHandler>(ApplicationMemoryArena, WAYLAND_MAX_INITHANDLER);

        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Init OK.");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Debug Mode.");
        #endif
        
        WaylandPerformanceCounterStart = SystemPlatformGetHighPerformanceCounter();
        WaylandPerformanceCounterFrequencyInSeconds = SystemPlatformGetHighPerformanceCounterFrequencyInSeconds();
    }
}

void AddWaylandInitHandler(WaylandInitHandlerPtr handler, ElemHandle handle)
{
    WaylandInitHandlers[WaylandCurrentInitIndex++] = 
    {
        .Function = handler,
        .Handle = handle,
        .NextIndex = 0
    };
}

void RemoveWaylandInitHandler(WaylandInitHandlerPtr handler)
{
    // TODO:
}

void WaylandXdgPingHandler(void* data, xdg_wm_base* shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

void WaylandRegistryHandler(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version) 
{
    if (SystemFindSubString(interface, wl_compositor_interface.name) != -1) 
    {
        WaylandCompositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) 
    {
        WaylandShell = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(WaylandShell, &WaylandXdgListener, nullptr);
    }
}

ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler)
{
    if (logHandler)
    {
        SystemRegisterLogHandler(logHandler);
    } 
}

ElemAPI ElemSystemInfo ElemGetSystemInfo()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto executablePath = SystemPlatformGetExecutablePath(stackMemoryArena);
    auto environment = SystemPlatformGetEnvironment(stackMemoryArena);
                      
    auto lastIndex = SystemLastIndexOf(executablePath, environment->PathSeparator);
    SystemAssert(lastIndex != -1);

    auto applicationPath = SystemPushArray<char>(stackMemoryArena, lastIndex + 2);
    SystemCopyBuffer(applicationPath, executablePath.Slice(0, lastIndex + 1));

    return
    {
        .Platform = ElemPlatform_Linux,
        .ApplicationPath = applicationPath.Pointer,
        .SupportMultiWindows = true
    };
}

ElemAPI int32_t ElemRunApplication(const ElemRunApplicationParameters* parameters)
{
    InitWaylandApplicationMemory();

    auto parametersCopy = SystemDuplicateBuffer(ApplicationMemoryArena, ReadOnlySpan<ElemRunApplicationParameters>((ElemRunApplicationParameters*)parameters, 1));

    WaylandDisplay = wl_display_connect(nullptr);
    SystemAssert(WaylandDisplay);

    auto registry = wl_display_get_registry(WaylandDisplay);
    SystemAssert(registry);

    wl_registry_add_listener(registry, &WaylandRegistryListener, nullptr);

    wl_display_roundtrip(WaylandDisplay);
    wl_registry_destroy(registry);

    SystemAssert(WaylandCompositor);
    SystemAssert(WaylandShell);

    if (parameters->InitHandler)
    {
        parameters->InitHandler(parameters->Payload);
    }

    waylandApplicationCanRun = true;

    for (uint32_t i = 0; i < WaylandCurrentInitIndex; i++)
    {
        auto handler = WaylandInitHandlers[i];
        handler.Function(handler.Handle);
    }

    while (wl_display_dispatch(WaylandDisplay) != -1 && waylandApplicationCanRun) 
    {
    }
    
    if (parameters->FreeHandler)
    {
        parameters->FreeHandler(parameters->Payload);
    }

    wl_display_disconnect(WaylandDisplay);

    return 0;
}

ElemAPI void ElemExitApplication(int32_t exitCode)
{
    waylandApplicationCanRun = false;
}
