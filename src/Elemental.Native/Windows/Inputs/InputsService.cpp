#include "HidInputDevice.h"
#include "SystemInputs.h"

#include <vector>

#define GAMEPAD_USAGE_PAGE 0x01
#define GAMEPAD_USAGE_ID   0x05

static InputState* globalInputState = nullptr;
static HCMNOTIFICATION globalNotificationHandle = nullptr;

// TODO: Review data structure
std::vector<HidInputDevice> globalHidInputDevices;

void RegisterHidDevice(HANDLE hidDevice)
{
    // HACK: Temporary
    // For now it seems that we have multiple hid devices for the same controller :/
    if (globalHidInputDevices.size() > 0)
    {
        //return;
    }

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

    LogDebugMessage(LogMessageCategory_Inputs, L"Gamepad connected: (ProductID: %d, VendorID: %d)", attrib.ProductID, attrib.VendorID);
    auto convertHidInputDeviceDataFunctionPointer = GetConvertHidInputDeviceDataFuncPtr(attrib.VendorID, attrib.ProductID);

    if (convertHidInputDeviceDataFunctionPointer == nullptr)
    {
        LogWarningMessage(LogMessageCategory_Inputs, L"Cannot find an HID Input Device data converter function for: (ProductID: %d, VendorID: %d)", attrib.ProductID, attrib.VendorID);
        CloseHandle(hidDevice);
        return;
    }

    HidInputDevice hidInputDevice = {};
    hidInputDevice.Device = hidDevice;
    hidInputDevice.DeviceId = (uint32_t)globalHidInputDevices.size();
    hidInputDevice.Event = CreateEvent(nullptr, true, false, nullptr);
    hidInputDevice.InputDataConvertFunction = convertHidInputDeviceDataFunctionPointer;
    
    OVERLAPPED overlapped = {};
    overlapped.hEvent = hidInputDevice.Event;
    hidInputDevice.Overlapped = overlapped;

    hidInputDevice.ReadBufferSizeInBytes = 1024;
    hidInputDevice.ReadBuffer = new uint8_t[hidInputDevice.ReadBufferSizeInBytes];
    ZeroMemory(hidInputDevice.ReadBuffer, hidInputDevice.ReadBufferSizeInBytes);
            
    ReadFile(hidInputDevice.Device, hidInputDevice.ReadBuffer, hidInputDevice.ReadBufferSizeInBytes, nullptr, &hidInputDevice.Overlapped);

    globalHidInputDevices.push_back(hidInputDevice);
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
        LogDebugMessage(LogMessageCategory_Inputs, L"Disconnected...");

        // TODO: Unregister device, free memory, etc...
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI InputThread(LPVOID lpParam)
{
    // TODO: We should get a copy of the device list here at each iteration
    // otherwise this could cause some multi threading issues

    HANDLE eventsToWait[32];

    while (globalInputState) 
    {
        auto inputDevicesCount = (int32_t)globalHidInputDevices.size();
        assert(inputDevicesCount <= 32);

        if (inputDevicesCount == 0)
        {
            Sleep(1000);
            continue;
        }

        for (int32_t i = 0; i < inputDevicesCount; i++)
        {
            eventsToWait[i] = globalHidInputDevices[(size_t)i].Event;
        }

        auto waitResult = WaitForMultipleObjects((DWORD)inputDevicesCount, eventsToWait, false, INFINITE);

        if (waitResult != WAIT_FAILED)
        {
            int32_t deviceIndex = (int32_t)(waitResult - WAIT_OBJECT_0);
            auto hidInputDevice = globalHidInputDevices[(size_t)deviceIndex];

            ReadFile(hidInputDevice.Device, hidInputDevice.ReadBuffer, hidInputDevice.ReadBufferSizeInBytes, nullptr, &hidInputDevice.Overlapped);

            DWORD bytesRead = 0;
            if (GetOverlappedResult(hidInputDevice.Device, &hidInputDevice.Overlapped, &bytesRead, true))
            {
                if (!globalInputState)
                {
                    return 0;
                }

                hidInputDevice.InputDataConvertFunction(globalInputState, 0, hidInputDevice.ReadBuffer, bytesRead);
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
    // TODO: Close thread

    for (size_t i = 0; i < globalHidInputDevices.size(); i++)
    {
        auto device = globalHidInputDevices[i];
        delete[] device.ReadBuffer;

        CloseHandle(device.Device);
    }

    if (globalNotificationHandle != nullptr)
    {
        AssertIfFailed(CM_Unregister_Notification(globalNotificationHandle));
        globalNotificationHandle = nullptr;
    }
    
    FreeInputState(globalInputState);
    globalInputState = nullptr;
}
    
DllExport InputState Native_GetInputState(void* applicationPointer)
{
    return *globalInputState;
}
