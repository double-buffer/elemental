#include "WindowsCommon.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "NativeApplicationService.cpp"
#include "Graphics/GraphicsService.cpp"
#include "Graphics/Direct3D12/Direct3D12GraphicsService.cpp"