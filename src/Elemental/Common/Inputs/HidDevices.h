#pragma once

#include "Elemental.h"
#include "SystemSpan.h"

bool IsHidDeviceSupported(uint32_t vendorId, uint32_t productId);
void ProcessHidDeviceData(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds);
