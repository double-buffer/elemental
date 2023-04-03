#include "WindowsCommon.h"
#include "../../Common/Elemental.h"
#include "../../Common/SystemFunctions.h"
#include "../../Common/HidDevices.h"

#define GAMEPAD_USAGE_PAGE 0x01
#define GAMEPAD_USAGE_ID   0x05

static InputState globalInputState;
static HCMNOTIFICATION globalNotificationHandle = nullptr;

HANDLE tempHidDevice;

void RegisterHidDevice(HANDLE hidDevice)
{
    PHIDP_PREPARSED_DATA preparsedData;
        
    if (!HidD_GetPreparsedData(hidDevice, &preparsedData)) 
    {
        CloseHandle(hidDevice);
        return;
    }

    HIDP_CAPS capabilities;
    if (HidP_GetCaps(preparsedData, &capabilities) != HIDP_STATUS_SUCCESS) 
    {
        HidD_FreePreparsedData(preparsedData);
        CloseHandle(hidDevice);
        return;
    }
        
    HidD_FreePreparsedData(preparsedData);

    if (capabilities.UsagePage != GAMEPAD_USAGE_PAGE || capabilities.Usage != GAMEPAD_USAGE_ID) 
    {
        CloseHandle(hidDevice);
        return;
    }

    HIDD_ATTRIBUTES attrib = {};
    attrib.Size = sizeof(attrib);

    if (!HidD_GetAttributes(hidDevice, &attrib))
    {
        CloseHandle(hidDevice);
        return;
    }

    printf("Gamepad connected: (ProductID: %d, VendorID: %d)\n", attrib.ProductID, attrib.VendorID);
    auto convertHidInputDeviceDataFunctionPointer = GetConvertHidInputDeviceDataFuncPtr(attrib.VendorID, attrib.ProductID);

    if (convertHidInputDeviceDataFunctionPointer == nullptr)
    {
        printf("Cannot find an HID Input Device data converter function for: (ProductID: %d, VendorID: %d)\n", attrib.ProductID, attrib.VendorID);
        CloseHandle(hidDevice);
        return;
    }

    // HACK: To remove
    tempHidDevice = hidDevice;
}

uint32_t HidDevice_NotificationCallback(HCMNOTIFICATION handle, void* context, CM_NOTIFY_ACTION action, PCM_NOTIFY_EVENT_DATA eventData, uint32_t eventDataSize)
{
    if (eventData->FilterType != CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE || !IsEqualGUID(eventData->u.DeviceInterface.ClassGuid, GUID_DEVINTERFACE_HID))
    {
        return ERROR_SUCCESS;
    }

    auto hidDevice = CreateFile(eventData->u.DeviceInterface.SymbolicLink, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

    if (hidDevice != INVALID_HANDLE_VALUE)
    {
        RegisterHidDevice(hidDevice);
    }
    else
    {
        printf("Disconnected...\n");
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI InputThread(LPVOID lpParam)
{
    auto event = CreateEvent(NULL, TRUE, FALSE, NULL);
    OVERLAPPED overlapped = {};
    overlapped.hEvent = event;

    BYTE readBuffer[1024];
    ZeroMemory(readBuffer, 1024);
    DWORD bytesRead;
    DWORD result;

    while (true) 
    {
        printf("Loop\n");

        ReadFile(tempHidDevice, readBuffer, sizeof(readBuffer), &bytesRead, &overlapped);

        auto waitResult = WaitForMultipleObjects(1, &event, FALSE, INFINITE);

        if (waitResult >= WAIT_OBJECT_0 && waitResult == WAIT_OBJECT_0)
        { 
            if (GetOverlappedResult(tempHidDevice, &overlapped, &bytesRead, true))
            {
                // BUG: There seems to be one byte missing buttons are not aligned
                struct XboxOneWirelessGamepadReport* inputData = (struct XboxOneWirelessGamepadReport*)(readBuffer);

                printf("%d %d\n", inputData->LeftStickX, inputData->Buttons);

                int currentIndex = 0;
                printf("%d ", readBuffer[currentIndex]);
                currentIndex ++;

                printf("%d ", *(uint16_t*)(&readBuffer[currentIndex]));
                currentIndex += 2;
                
                printf("%d ", *(uint16_t*)(&readBuffer[currentIndex]));
                currentIndex += 2;
                
                printf("%d ", *(uint16_t*)(&readBuffer[currentIndex]));
                currentIndex += 2;
                
                printf("%d ", *(uint16_t*)(&readBuffer[currentIndex]));
                currentIndex += 2;
                
                printf("%d ", *(uint16_t*)(&readBuffer[currentIndex]));
                currentIndex += 2;
                
                printf("%d ", *(uint16_t*)(&readBuffer[currentIndex]));
                currentIndex += 2;

                printf("%d ", readBuffer[currentIndex]);
                currentIndex ++;
                
                printf("%d ", *(uint16_t*)(&readBuffer[currentIndex]));
                currentIndex += 2;

                printf("\n");

                ConvertHidInputDeviceData_XboxOneWirelessGamepad(globalInputState, 0, readBuffer, bytesRead);
            }
        }
    }

    return 0;
}

DllExport void Native_InitInputsService()
{
    globalInputState = InitInputState();

    // Set up a device information set for gamepad devices
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        // Handle error
        return;
    }

    // Enumerate the gamepad devices in the device information set
    DWORD index = 0;
    SP_DEVICE_INTERFACE_DATA interfaceData;
    interfaceData.cbSize = sizeof(interfaceData);
    while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_HID, index, &interfaceData))
    {
        // Retrieve the symbolic link name for the gamepad device
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo, &interfaceData, NULL, 0, &requiredSize, NULL);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            // Handle error
            break;
        }

        PSP_DEVICE_INTERFACE_DETAIL_DATA interfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, requiredSize);
        if (interfaceDetailData == NULL)
        {
            // Handle error
            break;
        }

        interfaceDetailData->cbSize = sizeof(*interfaceDetailData);
        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &interfaceData, interfaceDetailData, requiredSize, NULL, NULL))
        {
            // Handle error
            LocalFree(interfaceDetailData);
            break;
        }

        HANDLE hDev = CreateFile(interfaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

        if (hDev != INVALID_HANDLE_VALUE)
        {
            RegisterHidDevice(hDev);
        }

        index++;
    }

    CM_NOTIFY_FILTER filter = {};
    filter.cbSize = sizeof(filter);
    filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    filter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_HID;
    filter.Flags = 0;

    AssertIfFailed(CM_Register_Notification(&filter, NULL, (PCM_NOTIFY_CALLBACK)&HidDevice_NotificationCallback, &globalNotificationHandle));
    CreateThread(NULL, 0, InputThread, NULL, 0, NULL);
}
    
DllExport void Native_FreeInputsService()
{
    FreeInputState(globalInputState);
    
    // TODO: Close all handles
    //CloseHandle(hidDevice);

    if (globalNotificationHandle != nullptr)
    {
        AssertIfFailed(CM_Unregister_Notification(globalNotificationHandle));
        globalNotificationHandle = nullptr;
    }
}
    
DllExport InputState Native_GetInputState(void* applicationPointer)
{
    return globalInputState;
}