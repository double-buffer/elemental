#pragma once

#if _WINDOWS
#define DllExport extern "C" __declspec(dllexport)
#else
#define DllExport extern "C" __attribute__((visibility("default"))) 
#endif