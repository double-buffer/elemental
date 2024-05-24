#include "WaylandApplication.h"
#include "WaylandInputs.h"
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
wl_output* WaylandOutput = nullptr;
wl_compositor* WaylandCompositor = nullptr;
wl_shm* WaylandShm = nullptr;
libdecor* WaylandLibdecor = nullptr;
bool waylandApplicationCanRun = false;

uint32_t WaylandOutputRefreshRate = 60;
float WaylandOutputScale = 1.0f;

wl_surface* WaylandCursorSurface = nullptr;
wl_cursor_image* WaylandCursorImage = nullptr;

void WaylandInitDefaultCursor()
{
    // TODO: Get the cursor settings. See: https://gitlab.freedesktop.org/libdecor/libdecor/-/blob/master/src/cursor-settings.c
    const char* cursorName = "left_ptr";
    int32_t cursorSize = 24;

    struct wl_cursor_theme *cursor_theme = wl_cursor_theme_load(NULL, cursorSize * WaylandOutputScale, WaylandShm);
    SystemAssert(cursor_theme);

    struct wl_cursor *cursor = wl_cursor_theme_get_cursor(cursor_theme, cursorName);
    SystemAssert(cursor);

    WaylandCursorImage = cursor->images[0];
    struct wl_buffer *buffer = wl_cursor_image_get_buffer(WaylandCursorImage);

    WaylandCursorSurface = wl_compositor_create_surface(WaylandCompositor);
    wl_surface_set_buffer_scale(WaylandCursorSurface, WaylandOutputScale);
    
    wl_surface_attach(WaylandCursorSurface, buffer, 0, 0);
    wl_surface_damage(WaylandCursorSurface, 0, 0, WaylandCursorImage->width, WaylandCursorImage->height);
    wl_surface_commit(WaylandCursorSurface);
}

void WaylandSetDefaultCursor(wl_pointer* pointer, uint32_t serial)
{
    SystemAssert(WaylandCursorSurface);
    wl_pointer_set_cursor(pointer, serial, WaylandCursorSurface, WaylandCursorImage->hotspot_x, WaylandCursorImage->hotspot_y);
}

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

void WaylandOutputModeHandler(void* data, wl_output* output, uint32_t flags, int width, int height, int refresh)
{
    if (flags & WL_OUTPUT_MODE_CURRENT) 
    {
        WaylandOutputRefreshRate = SystemRoundUp((float)refresh / 1000.0f);
    }
}

void WaylandOutputScaleHandler(void* data, wl_output* output, int32_t factor)
{
    WaylandOutputScale = factor;
    WaylandInitDefaultCursor();
}

void WaylandLibdecorErrorHandler(libdecor* context, libdecor_error error, const char* message)
{
    SystemLogErrorMessage(ElemLogMessageCategory_Application, message);
}

void WaylandRegistryHandler(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version) 
{
    if (SystemFindSubString(interface, wl_compositor_interface.name) != -1) 
    {
        WaylandCompositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, version);
    }
    else if (SystemFindSubString(interface, wl_output_interface.name) != -1) 
    {
        WaylandOutput = (wl_output*)wl_registry_bind(registry, id, &wl_output_interface, 2);
        wl_output_add_listener(WaylandOutput, &WaylandOutputListener, nullptr);
    }
    else if (SystemFindSubString(interface, wl_shm_interface.name) != -1) 
    {
        WaylandShm = (wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, version);
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
    SystemAssert(WaylandOutput);
    SystemAssert(WaylandShm);


    WaylandLibdecor = libdecor_new(WaylandDisplay, &WaylandLibdecorListener);

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
    
    while (libdecor_dispatch(WaylandLibdecor, -1) != -1 && waylandApplicationCanRun)
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
