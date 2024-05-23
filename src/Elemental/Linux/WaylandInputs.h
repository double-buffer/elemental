#pragma once

#include "Elemental.h"

void InitWaylandInputs(ElemWindow window);

void WaylandKeyboardKeymapHandler(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size) {}
void WaylandKeyboardEnterHandler(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys) {}
void WaylandKeyboardLeaveHandler(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface) {}
void WaylandKeyboardKeyHandler(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
void WaylandKeyboardModifiersHandler(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {}
void WaylandKeyboardRepeatInfoHandler(void *data, wl_keyboard* keyboard, int32_t rate, int32_t delay) {} // TODO: Override that one

const wl_keyboard_listener WaylandKeyboardListener = 
{
    .keymap = WaylandKeyboardKeymapHandler,
    .enter = WaylandKeyboardEnterHandler,
    .leave = WaylandKeyboardLeaveHandler,
    .key = WaylandKeyboardKeyHandler,
    .modifiers = WaylandKeyboardModifiersHandler,
    .repeat_info = WaylandKeyboardRepeatInfoHandler
};

void WaylandPointerEnterHandler(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy) {}
void WaylandPointerLeaveHandler(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface) {}
void WaylandPointerMotionHandler(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {}
void WaylandPointerButtonHandler(void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
void WaylandPointerAxisHandler(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
void WaylandPointerFrameHandler(void* data, wl_pointer* pointer) {}
void WaylandPointerAxisSourceHandler(void* data, wl_pointer* pointer, uint32_t axis_source) {}
void WaylandPointerAxisStopHandler(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis) {}
void WaylandPointerAxisDiscreteHandler(void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete) {}
void WaylandPointerAxisValue120Handler(void* data, wl_pointer* pointer, uint32_t axis, int32_t value120) {}
void WaylandPointerAxisRelativeDirectionHandler(void* data, wl_pointer* pointer, uint32_t axis, uint32_t direction) {}

const wl_pointer_listener WaylandPointerListener = 
{
    .enter = WaylandPointerEnterHandler,
    .leave = WaylandPointerLeaveHandler,
    .motion = WaylandPointerMotionHandler,
    .button = WaylandPointerButtonHandler,
    .axis = WaylandPointerAxisHandler,
    .frame = WaylandPointerFrameHandler,
    .axis_source = WaylandPointerAxisSourceHandler,
    .axis_stop = WaylandPointerAxisStopHandler,
    .axis_discrete = WaylandPointerAxisDiscreteHandler,
    .axis_value120 = WaylandPointerAxisValue120Handler,
    .axis_relative_direction = WaylandPointerAxisRelativeDirectionHandler
};

void WaylandRelativePointerMotionHandler(void* data, zwp_relative_pointer_v1* pointer, uint32_t time_hi, uint32_t time_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel);

const zwp_relative_pointer_v1_listener WaylandRelativePointerListener = 
{
    .relative_motion = WaylandRelativePointerMotionHandler
};

void WaylandSeatCapabilitiesHandler(void* data, wl_seat* seat, uint32_t capabilities);
void WaylandSeatNameHandler(void* data, wl_seat* wl_seat, const char* name) {}

// Seat listener
const wl_seat_listener WaylandSeatListener = 
{
    .capabilities = WaylandSeatCapabilitiesHandler,
    .name = WaylandSeatNameHandler,
};

void WaylandInputRegistryGlobalHandler(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version);

wl_registry_listener WaylandInputRegistryListener = 
{
    .global = WaylandInputRegistryGlobalHandler,
    .global_remove = nullptr,
};
