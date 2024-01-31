#include "HidInputDevice.h"

static InputState* globalInputState = nullptr;

static HidInputDevice hidInputDevices[64];
static uint32_t hidInputDevicesCount = 0;

void InputReportCallback(void* context, IOReturn result, void* sender, IOHIDReportType reportType, uint32_t reportID, uint8_t* report, CFIndex reportLength) 
{
    auto inputDevice = (HidInputDevice*)context;
    inputDevice->InputDataConvertFunction(globalInputState, inputDevice->DeviceId, report, reportLength);
}

void DeviceMatchingCallback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) 
{
    // TODO: Handle free memory!!!!

    auto usagePageRef = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey));
    uint32_t usagePage;
    CFNumberGetValue((CFNumberRef)usagePageRef, kCFNumberIntType, &usagePage);
    
    auto usageRef = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsageKey));
    uint32_t usage;
    CFNumberGetValue((CFNumberRef)usageRef, kCFNumberIntType, &usage);

    if (usagePage == kHIDPage_GenericDesktop && usage == kHIDUsage_GD_GamePad) 
    {
        auto nameRef = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
        auto name = CFStringGetCStringPtr(nameRef, kCFStringEncodingUTF8);

        uint32_t productID;
        auto productIDRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
        CFNumberGetValue((CFNumberRef)productIDRef, kCFNumberIntType, &productID);

        uint32_t vendorID;
        auto vendorIDRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
        CFNumberGetValue((CFNumberRef)vendorIDRef, kCFNumberIntType, &vendorID);
        
        printf("Gamepad connected: %s (ProductID: %u, VendorID: %u)\n", name, productID, vendorID);

        auto convertHidInputDeviceDataFunction = GetConvertHidInputDeviceDataFuncPtr(vendorID, productID);

        if (!convertHidInputDeviceDataFunction)
        {
            printf("warning: Cannot find an HID Input device data converter function.\n");
            return;
        }

        CFTypeRef reportSizeNumber = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDMaxInputReportSizeKey));

        if (reportSizeNumber == nullptr) 
        {
            return;
        }

        CFIndex reportSize = (CFIndex)reportSizeNumber;

        HidInputDevice hidInputDevice = {};
        hidInputDevice.DeviceId = hidInputDevicesCount;
        hidInputDevice.Device = device;
        hidInputDevice.InputDataConvertFunction = convertHidInputDeviceDataFunction;
        CFNumberGetValue((CFNumberRef)reportSizeNumber, kCFNumberIntType, &hidInputDevice.ReadBufferSizeInBytes);
        hidInputDevice.ReadBuffer = new uint8_t[hidInputDevice.ReadBufferSizeInBytes];

        hidInputDevices[hidInputDevicesCount++] = hidInputDevice;
        
        IOHIDDeviceRegisterInputReportCallback(device, hidInputDevice.ReadBuffer, hidInputDevice.ReadBufferSizeInBytes, InputReportCallback, &hidInputDevices[hidInputDevice.DeviceId]);
    }
}

void DeviceRemovalCallback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) 
{
    // Handle device removal callback
    printf("Device removed\n");
}

void* CheckInputDevicesThread(void* parameters)
{
    // TODO: Free Object
    auto hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    
    uint32_t entryValue = kHIDPage_GenericDesktop;
    CFStringRef primaryUsagePageKey = CFStringCreateWithCString(kCFAllocatorDefault, kIOHIDPrimaryUsagePageKey, kCFStringEncodingUTF8);
    CFNumberRef primaryUsagePageValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &entryValue);

    entryValue = kHIDUsage_GD_GamePad;
    CFStringRef primaryUsageKey = CFStringCreateWithCString(kCFAllocatorDefault, kIOHIDPrimaryUsageKey, kCFStringEncodingUTF8);
    CFNumberRef primaryUsageValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &entryValue);

    CFMutableDictionaryRef deviceDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(deviceDictionary, primaryUsagePageKey, primaryUsagePageValue);
    CFDictionarySetValue(deviceDictionary, primaryUsageKey, primaryUsageValue);

    CFRelease(primaryUsagePageKey);
    CFRelease(primaryUsagePageValue);
    CFRelease(primaryUsageKey);
    CFRelease(primaryUsageValue);
   
    IOHIDManagerSetDeviceMatching(hidManager, deviceDictionary);
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, &DeviceMatchingCallback, nullptr);
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, &DeviceRemovalCallback, nullptr);
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
    CFRunLoopRun();

    IOHIDManagerUnscheduleFromRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, nullptr, nullptr);
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, nullptr, nullptr);
    IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
    CFRelease(deviceDictionary);
    CFRelease(hidManager);

    pthread_exit(0);
}

DllExport void Native_InitInputsService()
{
    globalInputState = InitInputState();
    //SystemCreateThread(CheckInputDevicesThread);
}
    
DllExport void Native_FreeInputsService()
{
    FreeInputState(globalInputState);
    
    for (uint32_t i = 0; i < hidInputDevicesCount; i++)
    {
        delete hidInputDevices[i].ReadBuffer;
    }
}
    
DllExport InputState Native_GetInputState(void* applicationPointer)
{
    return *globalInputState;
}

DllExport void* Native_CreateInputsQueue()
{
    return CreateNativeInputsQueue();
}

DllExport void Native_FreeInputsQueue(void* inputsQueue)
{
    FreeNativeInputsQueue((NativeInputsQueue*)inputsQueue);
}

DllExport void Native_ReadInputsQueue(void* inputsQueue, InputsValue* inputsValues, int32_t* inputsValueCount)
{
    inputsValues[0] = {};
    inputsValues[0].Id = GamepadButton1;
    inputsValues[0].Value = 0.5f;
    inputsValues[0].Timestamp = 123;

    *inputsValueCount = 0;
}
